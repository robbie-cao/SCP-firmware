/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions and utility functions for the programming CMN-Cyprus RN SAM.
 */

#include <internal/cmn_cyprus_ctx.h>
#include <internal/cmn_cyprus_reg.h>
#include <internal/cmn_cyprus_rnsam.h>
#include <internal/cmn_cyprus_utils.h>

#include <mod_cmn_cyprus.h>

#include <fwk_assert.h>
#include <fwk_log.h>
#include <fwk_math.h>
#include <fwk_status.h>

#include <inttypes.h>
#include <stdint.h>

#define MAX_NON_HASH_REGION_COUNT 64

#define CMN_CYPRUS_RNSAM_STATUS_UNSTALL               UINT64_C(0x02)
#define CMN_CYPRUS_RNSAM_STATUS_USE_DEFAULT_TARGET_ID UINT64_C(0x01)

#define CMN_CYPRUS_RNSAM_NON_HASH_TGT_NODEID_ENTRY_BITS_WIDTH  12
#define CMN_CYPRUS_RNSAM_NON_HASH_TGT_NODEID_ENTRY_MASK        UINT64_C(0x0FFF)
#define CMN_CYPRUS_RNSAM_NON_HASH_TGT_NODEID_ENTRIES_PER_GROUP 4

/* RN SAM non-hashed region range comparision */
#define CMN_CYPRUS_RNSAM_UNIT_INFO_NONHASH_RANGE_COMP_EN_MASK \
    UINT64_C(0x80000000)
#define CMN_CYPRUS_RNSAM_UNIT_INFO_NONHASH_RANGE_COMP_EN_POS 31

/* RN SAM memory region */
#define CMN_CYPRUS_RNSAM_REGION_ENTRY_TYPE_POS   2
#define CMN_CYPRUS_RNSAM_REGION_ENTRY_SIZE_POS   56
#define CMN_CYPRUS_RNSAM_REGION_ENTRY_BASE_POS   26
#define CMN_CYPRUS_RNSAM_REGION_ENTRY_BITS_WIDTH 64
#define CMN_CYPRUS_RNSAM_REGION_ENTRY_VALID      UINT64_C(0x01)

/* SCG/HTG target node ID */
#define CMN_CYPRUS_HNS_CACHE_GROUP_ENTRIES_PER_GROUP       4
#define CMN_CYPRUS_HNS_CACHE_GROUP_ENTRY_BITS_WIDTH        12
#define CMN_CYPRUS_RNSAM_SYS_CACHE_GRP_HN_CNT_POS(scg_grp) (8 * (scg_grp))

/* CAL Mode */
#define CMN_CYPRUS_RNSAM_SCG_HNS_CAL_MODE_EN    UINT64_C(0x01)
#define CMN_CYPRUS_RNSAM_SCG_HNS_CAL_MODE_SHIFT 16

/* RN SAM Hierarchical hashing */
#define CMN_CYPRUS_RNSAM_HIERARCHICAL_HASH_EN_POS         2
#define CMN_CYPRUS_RNSAM_HIERARCHICAL_HASH_EN_MASK        UINT64_C(0x01)
#define CMN_CYPRUS_RNSAM_HIER_ENABLE_ADDRESS_STRIPING_POS 3
#define CMN_CYPRUS_RNSAM_HIER_HASH_CLUSTERS_POS           8
#define CMN_CYPRUS_RNSAM_HIER_HASH_NODES_POS              16
#define CMN_CYPRUS_RNSAM_SN_MODE_SYS_CACHE_POS(scg_grp) \
    ((4 + ((scg_grp)*16)) % 64)
#define CMN_CYPRUS_RNSAM_TOP_ADDRESS_BIT0_POS(scg_grp) \
    ((0 + ((scg_grp)*24)) % 64)
#define CMN_CYPRUS_RNSAM_TOP_ADDRESS_BIT1_POS(scg_grp) \
    ((8 + ((scg_grp)*24)) % 64)
#define CMN_CYPRUS_RNSAM_TOP_ADDRESS_BIT2_POS(scg_grp) \
    ((16 + ((scg_grp)*24)) % 64)

#define CMN_CYPRUS_RNSAM_SYS_CACHE_GRP_SN_ATTR_ENTRIES_PER_GRP    4
#define CMN_CYPRUS_RNSAM_SYS_CACHE_GRP_SN_SAM_CFG_ENTRIES_PER_GRP 2

/*
 * Used by RNSAM CPA registers
 *
 * CPA - CCIX Port Aggregation
 * PAG - Port Aggregation Group
 * GRPID - Group ID
 */
#define CML_PORT_AGGR_MODE_CTRL_REGIONS_PER_GROUP    10
#define CML_PORT_AGGR_MODE_CTRL_PAG_WIDTH_PER_REGION 6
#define CML_PORT_AGGR_MODE_CTRL_PAG_GRPID_OFFSET     1
#define CML_PORT_AGGR_CTRL_CPAG_PER_GROUP            5
#define CML_PORT_AGGR_CTRL_NUM_CXG_PAG_WIDTH         12
#define CML_CPAG_BASE_INDX_WIDTH_PER_CPAG            8
#define CML_CPAG_BASE_INDX_CPAG_PER_GROUP            8

/*
 * RN SAM node type.
 */
enum sam_node_type {
    SAM_NODE_TYPE_HN_F = 0,
    SAM_NODE_TYPE_HN_I,
    SAM_NODE_TYPE_CXRA,
    SAM_NODE_TYPE_COUNT
};

#if FWK_LOG_LEVEL <= FWK_LOG_LEVEL_INFO
static const char *const mmap_type_name[] = {
    [MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO] = "I/O",
    [MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE] = "System Cache",
    [MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE_SUB] = "Sub-System Cache",
    [MOD_CMN_CYPRUS_REGION_TYPE_CCG] = "CCG",
};
#endif

/* Shared driver context pointer */
static struct cmn_cyprus_ctx *shared_ctx;

/*
 * Stall RN SAM requests and enable RN SAM to use default target ID.
 */
static void rnsam_stall(void)
{
    struct cmn_cyprus_rnsam_reg *rnsam;
    uint32_t rnsam_idx;

    for (rnsam_idx = 0; rnsam_idx < shared_ctx->rnsam_count; rnsam_idx++) {
        rnsam = shared_ctx->rnsam_table[rnsam_idx];
        rnsam->STATUS = (rnsam->STATUS & ~CMN_CYPRUS_RNSAM_STATUS_UNSTALL) |
            CMN_CYPRUS_RNSAM_STATUS_USE_DEFAULT_TARGET_ID;
    }
    __sync_synchronize();
}

/*
 * Unstall RN SAM requests and enable RN SAM to hash address bits and generate
 * target ID.
 */
static void rnsam_unstall(void)
{
    struct cmn_cyprus_rnsam_reg *rnsam;
    uint32_t rnsam_idx;

    for (rnsam_idx = 0; rnsam_idx < shared_ctx->rnsam_count; rnsam_idx++) {
        rnsam = shared_ctx->rnsam_table[rnsam_idx];
        rnsam->STATUS = (rnsam->STATUS | CMN_CYPRUS_RNSAM_STATUS_UNSTALL) &
            ~(CMN_CYPRUS_RNSAM_STATUS_USE_DEFAULT_TARGET_ID);
    }
    __sync_synchronize();
}

static bool get_rnsam_nonhash_range_comp_en_mode(void *rnsam_reg)
{
    struct cmn_cyprus_rnsam_reg *rnsam = rnsam_reg;
    return (rnsam->UNIT_INFO[0] &
            CMN_CYPRUS_RNSAM_UNIT_INFO_NONHASH_RANGE_COMP_EN_MASK) >>
        CMN_CYPRUS_RNSAM_UNIT_INFO_NONHASH_RANGE_COMP_EN_POS;
}

static void print_rnsam_config_info(void)
{
#if FWK_LOG_LEVEL <= FWK_LOG_LEVEL_INFO
    unsigned int idx;
    uint64_t base;
    const struct mod_cmn_cyprus_mem_region_map *region;

    FWK_LOG_INFO(MOD_NAME "Regions to be mapped in RN SAM:");

    for (idx = 0; idx < shared_ctx->config->mmap_count; idx++) {
        region = &shared_ctx->config->mmap_table[idx];

        /* Offset the base with chip address space based on chip-id */
        base =
            ((uint64_t)(
                 shared_ctx->config->chip_addr_space * shared_ctx->chip_id) +
             region->base);

        FWK_LOG_INFO(
            MOD_NAME "  [0x%llx - 0x%llx] %s",
            base,
            base + region->size - 1,
            mmap_type_name[region->type]);
    }
#endif
}

static void get_non_hashed_region_registers(
    struct cmn_cyprus_rnsam_reg *rnsam,
    volatile uint64_t **reg,
    volatile uint64_t **reg_cfg2,
    unsigned int region_idx)
{
    fwk_assert(region_idx < MAX_NON_HASH_REGION_COUNT);

    if (region_idx < NON_HASH_MEM_REG_COUNT) {
        *reg = &rnsam->NON_HASH_MEM_REGION[region_idx];
        *reg_cfg2 = &rnsam->NON_HASH_MEM_REGION_CFG2[region_idx];
    } else {
        *reg = &rnsam->NON_HASH_MEM_REGION_GRP2
                    [region_idx - NON_HASH_MEM_REG_COUNT];
        *reg_cfg2 = &rnsam->NON_HASH_MEM_REGION_CFG2_GRP2
                         [region_idx - NON_HASH_MEM_REG_COUNT];
    }
}

static void get_scg_region_registers(
    struct cmn_cyprus_rnsam_reg *rnsam,
    volatile uint64_t **reg,
    volatile uint64_t **reg_cfg2,
    unsigned int region_idx)
{
    fwk_assert(region_idx < MAX_SCG_COUNT);

    *reg = &rnsam->SYS_CACHE_GRP_REGION[region_idx];
    *reg_cfg2 = &rnsam->HASHED_TGT_GRP_CFG2_REGION[region_idx];
}

static void configure_rnsam_region(
    void *rnsam_reg,
    unsigned int region_idx,
    uint64_t base,
    uint64_t size,
    enum sam_node_type node_type,
    enum sam_type sam_type)
{
    bool prog_start_and_end_addr;
    uint64_t lsb_addr_mask;
    uint64_t value;
    struct cmn_cyprus_rnsam_reg *rnsam = rnsam_reg;
    volatile uint64_t *reg;
    volatile uint64_t *reg_cfg2;

    fwk_assert(rnsam_reg);

    if (sam_type == SAM_TYPE_NON_HASH_MEM_REGION) {
        /*
         * Get the pointer to non-hashed memory region registers in
         * RN SAM register.
         */
        get_non_hashed_region_registers(rnsam, &reg, &reg_cfg2, region_idx);
    } else if (sam_type == SAM_TYPE_SYS_CACHE_GRP_REGION) {
        /*
         * Get the pointer to syscache group memory region registers in
         * RN SAM register.
         */
        get_scg_region_registers(rnsam, &reg, &reg_cfg2, region_idx);
    } else {
        FWK_LOG_ERR(MOD_NAME "Error! Unexpected sam_type");
        fwk_unexpected();
        return;
    }

    /* Check if the start and end address has to be programmed */
    prog_start_and_end_addr = (sam_type == SAM_TYPE_NON_HASH_MEM_REGION) ?
        get_rnsam_nonhash_range_comp_en_mode(rnsam) :
        get_rnsam_htg_range_comp_en_mode(rnsam);

    if ((!prog_start_and_end_addr) && ((base % size) != 0)) {
        FWK_LOG_ERR(
            MOD_NAME "Base: 0x%llx should align with Size: 0x%llx", base, size);
        fwk_unexpected();
    }

    /* Get the LSB mask from LSB bit position defining minimum region size */
    lsb_addr_mask = get_rnsam_lsb_addr_mask(rnsam, sam_type);

    value = CMN_CYPRUS_RNSAM_REGION_ENTRY_VALID;
    value |= node_type << CMN_CYPRUS_RNSAM_REGION_ENTRY_TYPE_POS;

    if (prog_start_and_end_addr) {
        value |= (base & ~lsb_addr_mask);
        /* Configure the end address of the region */
        *reg_cfg2 = (base + size - 1) & ~lsb_addr_mask;
    } else {
        /* Configure region size */
        value |= sam_encode_region_size(size)
            << CMN_CYPRUS_RNSAM_REGION_ENTRY_SIZE_POS;
        /* Configure region base */
        value |= (base / SAM_GRANULARITY)
            << CMN_CYPRUS_RNSAM_REGION_ENTRY_BASE_POS;
    }

    /* Program the register */
    *reg = value;
}

void set_non_hashed_region_target(
    struct cmn_cyprus_rnsam_reg *rnsam,
    uint32_t region_idx,
    unsigned int node_id)
{
    uint32_t register_idx;
    uint32_t bit_pos;

    register_idx =
        region_idx / CMN_CYPRUS_RNSAM_NON_HASH_TGT_NODEID_ENTRIES_PER_GROUP;

    bit_pos = CMN_CYPRUS_RNSAM_NON_HASH_TGT_NODEID_ENTRY_BITS_WIDTH *
        (region_idx % CMN_CYPRUS_RNSAM_NON_HASH_TGT_NODEID_ENTRIES_PER_GROUP);

    /* Clear the target node ID bitfield */
    rnsam->NON_HASH_TGT_NODEID[register_idx] &=
        ~(CMN_CYPRUS_RNSAM_NON_HASH_TGT_NODEID_ENTRY_MASK << bit_pos);

    /* Set the target node ID */
    rnsam->NON_HASH_TGT_NODEID[register_idx] |=
        (node_id & CMN_CYPRUS_RNSAM_NON_HASH_TGT_NODEID_ENTRY_MASK) << bit_pos;
}

static void configure_non_hashed_region(
    uint64_t region_base,
    uint64_t region_size,
    uint32_t region_idx,
    unsigned int target_node_id)
{
    unsigned int idx;
    uint64_t base;
    struct cmn_cyprus_rnsam_reg *rnsam;

    if (region_idx >= MAX_NON_HASH_REGION_COUNT) {
        FWK_LOG_ERR(
            MOD_NAME "Error! Invalid non-hashed region %lu", region_idx);
        FWK_LOG_ERR(
            MOD_NAME "Max non-hashed region supported is %u",
            MAX_NON_HASH_REGION_COUNT);
        fwk_unexpected();
    }

    /* Offset the base with chip address space based on chip-id */
    base =
        ((uint64_t)(shared_ctx->config->chip_addr_space * shared_ctx->chip_id) +
         region_base);

    /* Iterate through each RN SAM node and configure the region */
    for (idx = 0; idx < shared_ctx->rnsam_count; idx++) {
        rnsam = shared_ctx->rnsam_table[idx];

        configure_rnsam_region(
            rnsam,
            region_idx,
            base,
            region_size,
            SAM_NODE_TYPE_HN_I,
            SAM_TYPE_NON_HASH_MEM_REGION);

        set_non_hashed_region_target(rnsam, region_idx, target_node_id);
    }
}

/* Helper function to check if hns is inside the SCG/HTG square/rectangle */
static bool is_hns_inside_rect(
    struct node_pos hns_node_pos,
    const struct mod_cmn_cyprus_mem_region_map *region)
{
    struct node_pos region_hns_pos_start;
    struct node_pos region_hns_pos_end;

    region_hns_pos_start = region->hns_pos_start;
    region_hns_pos_end = region->hns_pos_end;

    if (((hns_node_pos.pos_x >= region_hns_pos_start.pos_x) &&
         (hns_node_pos.pos_y >= region_hns_pos_start.pos_y) &&
         (hns_node_pos.pos_x <= region_hns_pos_end.pos_x) &&
         (hns_node_pos.pos_y <= region_hns_pos_end.pos_y) &&
         (hns_node_pos.port_num <= region_hns_pos_end.port_num))) {
        if (hns_node_pos.pos_y == region_hns_pos_start.pos_y) {
            if (hns_node_pos.port_num >= region_hns_pos_start.port_num) {
                return true;
            } else {
                return false;
            }
        } else if (hns_node_pos.pos_y == region_hns_pos_end.pos_y) {
            if (hns_node_pos.port_num <= region_hns_pos_end.port_num) {
                return true;
            } else {
                return false;
            }
        }
        return true;
    }
    return false;
}

static void set_htg_target_hn_node(
    struct cmn_cyprus_rnsam_reg *rnsam,
    uint32_t hn_node_id,
    uint32_t hn_node_id_idx)
{
    uint32_t register_idx;
    uint32_t bit_pos;

    register_idx =
        hn_node_id_idx / CMN_CYPRUS_HNS_CACHE_GROUP_ENTRIES_PER_GROUP;

    bit_pos = CMN_CYPRUS_HNS_CACHE_GROUP_ENTRY_BITS_WIDTH *
        ((hn_node_id_idx % (CMN_CYPRUS_HNS_CACHE_GROUP_ENTRIES_PER_GROUP)));

    /* Only 16 registers are supported currently */
    fwk_assert(register_idx < 16);

    /* Configure target HN-F node ID */
    rnsam->SYS_CACHE_GRP_HN_NODEID[register_idx] += (uint64_t)hn_node_id
        << bit_pos;
}

static void set_htg_target_sn_node(
    struct cmn_cyprus_rnsam_reg *rnsam,
    uint32_t sn_node_id,
    uint32_t sn_node_id_idx)
{
    uint32_t register_idx;
    uint32_t bit_pos;

    register_idx =
        sn_node_id_idx / CMN_CYPRUS_HNS_CACHE_GROUP_ENTRIES_PER_GROUP;

    bit_pos = CMN_CYPRUS_HNS_CACHE_GROUP_ENTRY_BITS_WIDTH *
        ((sn_node_id_idx % (CMN_CYPRUS_HNS_CACHE_GROUP_ENTRIES_PER_GROUP)));

    /* Only 32 registers are supported currently */
    fwk_assert(register_idx < 32);

    /* Configure target SN node ID */
    rnsam->SYS_CACHE_GRP_SN_NODEID[register_idx] += (uint64_t)sn_node_id
        << bit_pos;
}

static void configure_scg_target_nodes(
    struct cmn_cyprus_rnsam_reg *rnsam,
    const struct mod_cmn_cyprus_mem_region_map *region,
    uint32_t scg_idx)
{
    uint8_t hns_idx;
    uint32_t hns_count_in_scg;
    uint32_t hns_nodeid;
    uint32_t hn_node_id_idx;
    unsigned int hns_ldid;
    const struct mod_cmn_cyprus_config *config;

    config = shared_ctx->config;
    hns_count_in_scg = 0;
    hn_node_id_idx = 0;

    /*
     * Iterate through each HN-S node and configure the target node ID if it
     * falls within the arbitrary SCG square/rectange.
     */
    for (hns_idx = 0; hns_idx < shared_ctx->hns_count; hns_idx++) {
        /* Skip isolated HN-S nodes */
        if (shared_ctx->hns_table[hns_idx].hns == 0) {
            continue;
        }

        hns_nodeid = get_node_id((void *)shared_ctx->hns_table[hns_idx].hns);
        hns_ldid =
            get_node_logical_id((void *)shared_ctx->hns_table[hns_idx].hns);

        if ((config->hns_cal_mode) && ((hns_nodeid % 2) == 1)) {
            /* Ignore odd node ids if cal mode is set */
            continue;
        }

        if (is_hns_inside_rect(
                shared_ctx->hns_table[hns_idx].node_pos, region)) {
            /* Configure target HN-F node ID */
            set_htg_target_hn_node(rnsam, hns_nodeid, hn_node_id_idx);

            /* Configure target SN node ID */
            set_htg_target_sn_node(
                rnsam, config->snf_table[hns_ldid], hn_node_id_idx);

            hns_count_in_scg++;
            hn_node_id_idx++;
        }
    }

    /* Configure the number of HN-S nodes in this syscache group */
    rnsam->SYS_CACHE_GRP_HN_COUNT |= ((uint64_t)hns_count_in_scg)
        << (CMN_CYPRUS_RNSAM_SYS_CACHE_GRP_HN_CNT_POS(scg_idx));
}

static void configure_scg_cal_mode(
    struct cmn_cyprus_rnsam_reg *rnsam,
    unsigned int scg_idx)
{
    /*
     * TO-DO: Add support for other CAL modes.
     * Only CAL2 mode is supported currently.
     */

    /* Enable the CAL mode for corresponding SCG */
    rnsam->SYS_CACHE_GRP_CAL_MODE |= shared_ctx->scg_enabled[scg_idx] *
        (CMN_CYPRUS_RNSAM_SCG_HNS_CAL_MODE_EN
         << (scg_idx * CMN_CYPRUS_RNSAM_SCG_HNS_CAL_MODE_SHIFT));
}

static void configure_scg_hier_hashing(
    struct cmn_cyprus_rnsam_reg *rnsam,
    unsigned int scg_idx,
    const struct mod_cmn_cyprus_hierarchical_hashing *hier_hash_cfg)
{
    unsigned int register_idx;
    unsigned int hns_count_per_cluster;
    unsigned int hns_count;
    unsigned int hns_cluster_count;

    /*
     * If CAL mode is enabled, only the even numbered HN-S nodes are
     * programmed. Hence, Reduce HN-S count by half if CAL mode is enabled.
     */
    if (shared_ctx->config->hns_cal_mode) {
        hns_count = (shared_ctx->hns_count / 2);
    } else {
        hns_count = shared_ctx->hns_count;
    }

    /* Total number of HN-S clusters */
    hns_cluster_count = hier_hash_cfg->hns_cluster_count;

    /* Number of HN-S nodes in a cluster */
    hns_count_per_cluster =
        (hns_count / hns_cluster_count) / shared_ctx->scg_count;

    /* Enable hierarchical hashing mode */
    rnsam->HASHED_TARGET_GRP_HASH_CNTL[scg_idx] =
        (CMN_CYPRUS_RNSAM_HIERARCHICAL_HASH_EN_MASK
         << CMN_CYPRUS_RNSAM_HIERARCHICAL_HASH_EN_POS);

    /*
     * Configure number of address bits needs to shuttered (removed) at
     * second hierarchy hash.
     */
    rnsam->HASHED_TARGET_GRP_HASH_CNTL[scg_idx] =
        (fwk_math_log2(hns_count_per_cluster)
         << CMN_CYPRUS_RNSAM_HIER_ENABLE_ADDRESS_STRIPING_POS);

    /* Configure the number of clusters */
    rnsam->HASHED_TARGET_GRP_HASH_CNTL[scg_idx] =
        (hier_hash_cfg->hns_cluster_count
         << CMN_CYPRUS_RNSAM_HIER_HASH_CLUSTERS_POS);

    /* Configure the number of HN-S nodes in each cluster */
    rnsam->HASHED_TARGET_GRP_HASH_CNTL[scg_idx] =
        (hns_count_per_cluster << CMN_CYPRUS_RNSAM_HIER_HASH_NODES_POS);

    register_idx =
        scg_idx / CMN_CYPRUS_RNSAM_SYS_CACHE_GRP_SN_ATTR_ENTRIES_PER_GRP;

    /* Configure the SN selection mode */
    rnsam->SYS_CACHE_GRP_SN_ATTR[register_idx] |=
        (hier_hash_cfg->sn_mode
         << CMN_CYPRUS_RNSAM_SN_MODE_SYS_CACHE_POS(scg_idx));

    register_idx =
        scg_idx / CMN_CYPRUS_RNSAM_SYS_CACHE_GRP_SN_SAM_CFG_ENTRIES_PER_GRP;

    /* Configure the top address bits for SCG */
    rnsam->SYS_CACHE_GRP_SN_SAM_CFG[register_idx] |=
        (hier_hash_cfg->top_address_bit0
         << CMN_CYPRUS_RNSAM_TOP_ADDRESS_BIT0_POS(scg_idx));

    rnsam->SYS_CACHE_GRP_SN_SAM_CFG[register_idx] |=
        (hier_hash_cfg->top_address_bit1
         << CMN_CYPRUS_RNSAM_TOP_ADDRESS_BIT1_POS(scg_idx));

    rnsam->SYS_CACHE_GRP_SN_SAM_CFG[register_idx] |=
        (hier_hash_cfg->top_address_bit2
         << CMN_CYPRUS_RNSAM_TOP_ADDRESS_BIT2_POS(scg_idx));
}

static void configure_scg_region(
    const struct mod_cmn_cyprus_mem_region_map *scg_region,
    uint32_t scg_idx)
{
    unsigned int idx;
    struct cmn_cyprus_rnsam_reg *rnsam;

    if (scg_idx >= MAX_SCG_COUNT) {
        FWK_LOG_ERR(MOD_NAME "Error! Invalid SCG region %lu", scg_idx);
        FWK_LOG_ERR(MOD_NAME "Max SCG region supported is %u ", MAX_SCG_COUNT);
        fwk_unexpected();
    }

    /* Iterate through each RN SAM node and configure the region */
    for (idx = 0; idx < shared_ctx->rnsam_count; idx++) {
        rnsam = shared_ctx->rnsam_table[idx];

        configure_rnsam_region(
            rnsam,
            scg_idx,
            scg_region->base,
            scg_region->size,
            SAM_NODE_TYPE_HN_F,
            SAM_TYPE_SYS_CACHE_GRP_REGION);

        /* Configure the target nodes for the SCG */
        configure_scg_target_nodes(rnsam, scg_region, scg_idx);

        if (shared_ctx->config->hns_cal_mode) {
            /* Configure the SCG CAL mode support */
            configure_scg_cal_mode(rnsam, scg_idx);
        }

        if (shared_ctx->config->hierarchical_hashing_enable) {
            configure_scg_hier_hashing(
                rnsam,
                scg_idx,
                &shared_ctx->config->hierarchical_hashing_config);
        }
    }

    /* Mark corresponding SCG as enabled in the context */
    shared_ctx->scg_enabled[scg_idx] = true;
}

static void program_rnsam_region(
    const struct mod_cmn_cyprus_mem_region_map *region)
{
    unsigned int region_idx;

    if (region->type == MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE_SUB) {
        /* Syscache sub-regions are handled by dedicated HN-S nodes */
        return;
    }

    switch (region->type) {
    case MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO:
        region_idx = shared_ctx->io_region_count++;
        /* Configure non-hashed region */
        configure_non_hashed_region(
            region->base, region->size, region_idx, region->node_id);
        break;

    case MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE:
        region_idx = shared_ctx->scg_count++;
        /* Configure SCG region */
        configure_scg_region(region, region_idx);
        break;

    default:
        fwk_trap();
    }
}

static unsigned int get_non_hashed_region_target_id(
    unsigned int region_idx,
    struct cmn_cyprus_rnsam_reg *rnsam)
{
    uint32_t register_idx;
    uint32_t bit_pos;

    register_idx =
        region_idx / CMN_CYPRUS_RNSAM_NON_HASH_TGT_NODEID_ENTRIES_PER_GROUP;

    bit_pos = CMN_CYPRUS_RNSAM_NON_HASH_TGT_NODEID_ENTRY_BITS_WIDTH *
        (region_idx % CMN_CYPRUS_RNSAM_NON_HASH_TGT_NODEID_ENTRIES_PER_GROUP);

    /* Return the target node ID programmed in the register */
    return (rnsam->NON_HASH_TGT_NODEID[register_idx] >> bit_pos) &
        CMN_CYPRUS_RNSAM_NON_HASH_TGT_NODEID_ENTRY_MASK;
}

static bool is_non_hashed_region_mapped(
    struct cmn_cyprus_rnsam_reg *rnsam,
    struct mod_cmn_cyprus_mem_region_map *mmap,
    uint32_t *region_index)
{
    int idx;
    unsigned int programmed_node_id;
    volatile uint64_t *reg;
    uint64_t lsb_addr_mask;

    lsb_addr_mask =
        get_rnsam_lsb_addr_mask(rnsam, SAM_TYPE_NON_HASH_MEM_REGION);

    /* Iterate through each non-hashed region mapped */
    for (idx = shared_ctx->io_region_count - 1; idx >= 0; idx--) {
        /* Get the non-hashed region register in the RN SAM */
        reg = &rnsam->NON_HASH_MEM_REGION[idx];

        /* Check if the programmed address region matches the given region */
        if (mmap->base == (*reg & ~lsb_addr_mask)) {
            /* Get the programmed target node ID */
            programmed_node_id = get_non_hashed_region_target_id(idx, rnsam);

            /* Calculate the target node ID */
            mmap->node_id &= CMN_CYPRUS_RNSAM_NON_HASH_TGT_NODEID_ENTRY_MASK;

            /*
             * Check if the programmed node ID matches the given target node ID.
             */
            if (programmed_node_id == mmap->node_id) {
                FWK_LOG_INFO(
                    MOD_NAME "Found IO region: %d mapped for Node: %u ",
                    idx,
                    mmap->node_id);

                /* Return the region index of the programmed region */
                *region_index = idx;
                return true;
            } else {
                FWK_LOG_ERR(
                    MOD_NAME
                    "Address: 0x%llx mapped to different node id:"
                    " %u than expected: %u\n",
                    mmap->base,
                    programmed_node_id,
                    mmap->node_id);
                fwk_unexpected();
            }
        }
    }

    return false;
}

static void update_io_region(
    struct mod_cmn_cyprus_mem_region_map *mmap,
    uint32_t region_idx)
{
    struct cmn_cyprus_rnsam_reg *rnsam;
    uint32_t idx;

    FWK_LOG_INFO(MOD_NAME "Updating IO region %ld", region_idx);
    FWK_LOG_INFO(
        MOD_NAME "  [0x%llx - 0x%llx] %s",
        mmap->base,
        mmap->base + mmap->size - 1,
        mmap_type_name[mmap->type]);

    /* Update the IO region in RN SAM */
    for (idx = 0; idx < shared_ctx->rnsam_count; idx++) {
        rnsam = shared_ctx->rnsam_table[idx];
        configure_rnsam_region(
            rnsam,
            region_idx,
            mmap->base,
            mmap->size,
            SAM_NODE_TYPE_HN_I,
            SAM_TYPE_NON_HASH_MEM_REGION);
    }
}

static int map_io_region(uint64_t base, size_t size, uint32_t node_id)
{
    uint32_t region_idx;
    struct cmn_cyprus_rnsam_reg *rnsam;
    struct mod_cmn_cyprus_mem_region_map mmap = {
        .base = base,
        .size = size,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = node_id,
    };

    rnsam_stall();

    /*
     * All the regions are identically mapped in all the RNSAMs. We can use only
     * one to check if it is already mapped.
     */
    rnsam = shared_ctx->rnsam_table[0];

    /* Check if the given non-hashed region has been mapped */
    if (is_non_hashed_region_mapped(rnsam, &mmap, &region_idx)) {
        /* Update the exisiting IO region in RN SAM */
        update_io_region(&mmap, region_idx);
    } else {
        FWK_LOG_INFO(MOD_NAME "Mapping IO region in RN SAM");
        FWK_LOG_INFO(
            MOD_NAME "  [0x%llx - 0x%llx] %s",
            base,
            base + size - 1,
            mmap_type_name[mmap.type]);

        /* Program the IO region in RN SAM */
        program_rnsam_region(&mmap);
    }

    rnsam_unstall();

    return FWK_SUCCESS;
}

static struct mod_cmn_cyprus_rnsam_memmap_api rnsam_memmap_api = {
    .map_io_region = map_io_region,
};

static void configure_ccg_region(
    struct cmn_cyprus_rnsam_reg *rnsam,
    uint64_t region_base,
    uint64_t region_size,
    uint32_t region_idx,
    unsigned int target_node_id)
{
    configure_rnsam_region(
        rnsam,
        region_idx,
        region_base,
        region_size,
        SAM_NODE_TYPE_CXRA,
        SAM_TYPE_NON_HASH_MEM_REGION);

    set_non_hashed_region_target(rnsam, region_idx, target_node_id);
}

static void program_rnsam_ccg_region(
    unsigned int target_node_id,
    const struct mod_cmn_cyprus_mem_region_map *region,
    unsigned int region_idx)
{
    unsigned int rnsam_idx;
    struct cmn_cyprus_rnsam_reg *rnsam;

    for (rnsam_idx = 0; rnsam_idx < shared_ctx->rnsam_count; rnsam_idx++) {
        rnsam = shared_ctx->rnsam_table[rnsam_idx];

        /* Configure the address range and the target node in RN SAM */
        configure_ccg_region(
            rnsam, region->base, region->size, region_idx, target_node_id);
    }
}

static void configure_rnsam_cpag(
    struct cmn_cyprus_rnsam_reg *rnsam,
    unsigned int region_idx,
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    unsigned int register_idx;
    unsigned int bit_pos;

    register_idx = (region_idx / CML_PORT_AGGR_MODE_CTRL_REGIONS_PER_GROUP);
    bit_pos =
        ((region_idx % CML_PORT_AGGR_MODE_CTRL_REGIONS_PER_GROUP) *
         CML_PORT_AGGR_MODE_CTRL_PAG_WIDTH_PER_REGION);

    /* Only 0-3 CML_PORT_AGGR_MODE_CTRL registers are defined */
    fwk_assert(register_idx < 4);

    /* Enable CPA mode for non-hashed memory region */
    rnsam->CML_PORT_AGGR_MODE_CTRL_REG[register_idx] |=
        (UINT64_C(0x1) << (bit_pos));

    /* Configure the CPAG ID */
    rnsam->CML_PORT_AGGR_MODE_CTRL_REG[register_idx] |=
        ((uint64_t)ccg_config->cpag_id
         << (bit_pos + CML_PORT_AGGR_MODE_CTRL_PAG_GRPID_OFFSET));

    /* Configure the number of CCG ports in the CPAG */
    register_idx = (ccg_config->cpag_id / CML_PORT_AGGR_CTRL_CPAG_PER_GROUP);
    rnsam->CML_PORT_AGGR_CTRL_REG[register_idx] |=
        (ccg_config->num_ports_in_cpag - 1)
        << ((ccg_config->cpag_id % CML_PORT_AGGR_CTRL_CPAG_PER_GROUP) *
            CML_PORT_AGGR_CTRL_NUM_CXG_PAG_WIDTH);

    /* Configure the port type as CML SMP */
    rnsam->CML_PORT_AGGR_CTRL_REG[register_idx] |= 0x1
        << (((ccg_config->cpag_id % CML_PORT_AGGR_CTRL_CPAG_PER_GROUP) *
             CML_PORT_AGGR_CTRL_NUM_CXG_PAG_WIDTH) +
            0x5u);

    /* Configure the target CCG node IDs within the CPAG */
    for (uint8_t idx = 0; idx < ccg_config->num_ports_in_cpag; idx++) {
        unsigned int ccg_ra_ldid = ccg_config->ldid[idx];
        unsigned int ccg_ra_node_id =
            shared_ctx->ccg_ra_reg_table[ccg_ra_ldid].node_id;

        /* Calculate the register index */
        register_idx =
            (ccg_config->cpag_id * ccg_config->num_ports_in_cpag + idx) /
            CMN_PORT_AGGR_GRP_PAG_TGTID_PER_GROUP;

        /* Calculate the bit position */
        bit_pos =
            (((ccg_config->cpag_id * ccg_config->num_ports_in_cpag + idx) *
              CMN_PORT_AGGR_GRP_PAG_TGTID_WIDTH) %
             CMN_PORT_AGGR_GRP_PAG_TGTID_WIDTH_PER_GROUP);

        /* Only 0-2 CML_PORT_AGGR_GRP registers are defined */
        fwk_assert(register_idx < 3);

        /* Configure the target node ID */
        rnsam->CML_PORT_AGGR_GRP_REG[register_idx] |= ((uint64_t)ccg_ra_node_id)
            << bit_pos;
    }

    /* Configure the CPAG base index */
    register_idx = (ccg_config->cpag_id / CML_CPAG_BASE_INDX_CPAG_PER_GROUP);
    bit_pos =
        ((ccg_config->cpag_id % CML_CPAG_BASE_INDX_CPAG_PER_GROUP) *
         CML_CPAG_BASE_INDX_WIDTH_PER_CPAG);

    /*
     * Note: The following calculation is based on the assumption that the
     * number of CCG ports per CPAG is the same for all the CPAGs in the mesh.
     */
    rnsam->CML_CPAG_BASE_INDX_GRP_REG[register_idx] &= ~(0x3F << bit_pos);
    rnsam->CML_CPAG_BASE_INDX_GRP_REG[register_idx] |=
        ((ccg_config->cpag_id * ccg_config->num_ports_in_cpag) << bit_pos);
}

static void program_rnsam_cpag(
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    unsigned int rnsam_idx;
    struct cmn_cyprus_rnsam_reg *rnsam;

    /* To-Do: Check if the number of CCG ports in CPAG is valid */

    for (rnsam_idx = 0; rnsam_idx < shared_ctx->rnsam_count; rnsam_idx++) {
        rnsam = shared_ctx->rnsam_table[rnsam_idx];

        /* Configure the CPA registers in RN SAM */
        configure_rnsam_cpag(rnsam, shared_ctx->io_region_count, ccg_config);
    }
}

static void setup_rnsam_ccg_regions(void)
{
    const struct mod_cmn_cyprus_config *config;
    const struct mod_cmn_cyprus_mem_region_map *region;
    const struct mod_cmn_cyprus_ccg_config *ccg_config;
    unsigned int ccg_idx, region_idx;
    unsigned int target_node_id, ccg_ra_ldid;

    config = shared_ctx->config;

    /* Iterate through each CCG configuration */
    for (ccg_idx = 0; ccg_idx < config->ccg_table_count; ccg_idx++) {
        /* Iterate through each remote memory map entry in the CCG config */
        for (region_idx = 0; region_idx < CMN_CYPRUS_MAX_RA_SAM_ADDR_REGION;
             region_idx++) {
            ccg_config = &config->ccg_config_table[ccg_idx];
            region = &ccg_config->remote_mmap_table[region_idx];

            if (region->size == 0) {
                /* Skip empty entries in the table */
                continue;
            }

            fwk_assert(region->type == MOD_CMN_CYPRUS_REGION_TYPE_CCG);

            FWK_LOG_INFO(
                MOD_NAME "  [0x%llx - 0x%llx] %s",
                region->base,
                region->base + region->size - 1,
                mmap_type_name[region->type]);

            /* Calculate the target node ID for the CCG region */
            ccg_ra_ldid = ccg_config->ldid[0];
            target_node_id = shared_ctx->ccg_ra_reg_table[ccg_ra_ldid].node_id;

            /* Program the CCG region in RN SAM */
            program_rnsam_ccg_region(
                target_node_id, region, shared_ctx->io_region_count);

            /* Program CPAG */
            if (ccg_config->cpa_mode) {
                program_rnsam_cpag(ccg_config);
            }

            /*
             * Increment the IO count as CCG region is programmed as non-hashed
             * region currently.
             */
            shared_ctx->io_region_count++;
        }
    }
}

void cmn_cyprus_setup_rnsam(struct cmn_cyprus_ctx *ctx)
{
    unsigned int region_idx;

    /* Initialize the shared context pointer */
    shared_ctx = ctx;

    rnsam_stall();

    print_rnsam_config_info();

    /* Configure the regions in RN SAM */
    for (region_idx = 0; region_idx < shared_ctx->config->mmap_count;
         region_idx++) {
        program_rnsam_region(&shared_ctx->config->mmap_table[region_idx]);
    }

    /* Program the CCG regions in RN SAM */
    setup_rnsam_ccg_regions();

    rnsam_unstall();

    FWK_LOG_INFO(MOD_NAME "RN SAM setup complete");
}

void get_rnsam_memmap_api(const void **api)
{
    *api = &rnsam_memmap_api;
}
