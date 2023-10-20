/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions and utility functions for programming CCG.
 */

#include <internal/cmn_cyprus_ccg.h>
#include <internal/cmn_cyprus_ctx.h>
#include <internal/cmn_cyprus_reg.h>
#include <internal/cmn_cyprus_utils.h>

#include <mod_cmn_cyprus.h>
#include <mod_timer.h>

#include <fwk_assert.h>
#include <fwk_log.h>
#include <fwk_math.h>
#include <fwk_status.h>

#include <inttypes.h>
#include <stdint.h>

#define SAM_ADDR_TARGET_HAID_SHIFT (52)
#define SAM_ADDR_REG_VALID_MASK    UINT64_C(0x8000000000000000)

#define MAX_AGENT_ID                 63
#define NUM_BITS_RESERVED_FOR_LINKID 8

#define EXP_RAID_TO_LDID_VALID_MASK (UINT64_C(1) << 15)
#define NUM_BITS_RESERVED_FOR_LDID  16

/* CCG Home Agent (HA) defines */
#define CCG_HA_RAID_TO_LDID_RNF_MASK (0x4000)

#define HNS_RN_PHYS_RN_ID_VALID_SHIFT_VAL     31
#define HNS_RN_PHYS_RN_LOCAL_REMOTE_SHIFT_VAL 16
#define REMOTE_CCG_NODE                       1

#define NUM_BITS_RESERVED_FOR_RAID  16
#define LDID_TO_EXP_RAID_VALID_MASK (UINT64_C(1) << 15)

/* SMP Mode related defines */
#define CCG_RA_CCPRTCL_LINK_CTRL_SMP_MODE_EN_SHIFT_VAL 16
#define CCG_HA_CCPRTCL_LINK_CTRL_SMP_MODE_EN_SHIFT_VAL 16

/* ULL to ULL Mode related defines */
#define CCLA_ULL_CTL_ULL_TO_ULL_MODE_EN_SHIFT_VAL 1
#define CCLA_ULL_CTL_SEND_VD_INIT_SHIFT_VAL       0
#define CCLA_ULL_STATUS_SEND_RX_ULL_STATE_MASK    UINT64_C(0x2)
#define CCLA_ULL_STATUS_SEND_TX_ULL_STATE_MASK    UINT64_C(0x1)
#define CCLA_ULL_STATUS_TIMEOUT                   UINT32_C(100)

/* CCG link control & status defines */
#define CCG_LINK_CTRL_EN_MASK              UINT64_C(0x0000000000000001)
#define CCG_LINK_CTRL_REQ_MASK             UINT64_C(0x0000000000000002)
#define CCG_LINK_CTRL_UP_MASK              UINT64_C(0x0000000000000004)
#define CCG_LINK_STATUS_ACK_MASK           UINT64_C(0x0000000000000001)
#define CCG_LINK_STATUS_DOWN_MASK          UINT64_C(0x0000000000000002)
#define CCG_CCPRTCL_LINK_CTRL_TIMEOUT      UINT32_C(100)
#define CCG_LINK_CTRL_DVMDOMAIN_REQ_MASK   UINT64_C(0x0000000000000008)
#define CCG_LINK_STATUS_DVMDOMAIN_ACK_MASK UINT64_C(0x0000000000000004)
#define CCG_CCPRTCL_LINK_DVMDOMAIN_TIMEOUT UINT32_C(100)

#define HNF_RN_PHYS_CPA_GRP_RA_SHIFT_VAL 17
#define HNF_RN_PHYS_CPA_EN_RA_SHIFT_VAL  30

/*
 * CCG Link UP stages.
 *
 * Each stage has an associated condition to be verified and this enum
 * is used to identify the condition.
 */
enum ccg_link_up_wait_cond {
    CCG_LINK_CTRL_EN_BIT_SET,
    CCG_LINK_CTRL_UP_BIT_CLR,
    CCG_LINK_STATUS_DWN_BIT_SET,
    CCG_LINK_STATUS_DWN_BIT_CLR,
    CCG_LINK_STATUS_ACK_BIT_SET,
    CCG_LINK_STATUS_ACK_BIT_CLR,
    CCG_LINK_STATUS_HA_DVMDOMAIN_ACK_BIT_SET,
    CCG_LINK_STATUS_RA_DVMDOMAIN_ACK_BIT_SET,
    CCG_LINK_UP_SEQ_COUNT,
};

/*
 * Structure to be passed to the timer API
 */
struct ccg_wait_condition_data {
    /* Link ID */
    uint8_t linkid;
    /* Condition to be verified for \ref linkid */
    enum ccg_link_up_wait_cond cond;
    /* CCG config data */
    const struct mod_cmn_cyprus_ccg_config *ccg_config;
    /* CCG port in a CPAG */
    uint8_t idx;
};

/* Shared driver context pointer */
static struct cmn_cyprus_ctx *shared_ctx;

static unsigned int get_local_ra_count(void)
{
    /* Return the max count among the RNs */
    if ((shared_ctx->rnf_count > shared_ctx->rnd_count) &&
        (shared_ctx->rnf_count > shared_ctx->rni_count)) {
        return shared_ctx->rnf_count;
    } else if (shared_ctx->rnd_count > shared_ctx->rni_count) {
        return shared_ctx->rnd_count;
    } else {
        return shared_ctx->rni_count;
    }
}

static void configure_ra_sam_region(
    struct cmn_cyprus_ccg_ra_reg *ccg_ra_reg,
    uint8_t region_idx,
    uint64_t base,
    uint64_t size,
    unsigned int target_haid)
{
    uint64_t blocks;

    /* Size must be a multiple of SAM_GRANULARITY */
    fwk_assert((size % (64 * FWK_KIB)) == 0);

    /* Size also must be a power of two */
    fwk_assert((size & (size - 1)) == 0);

    /*
     * TO-DO: Check if base is naturally aligned to the size of
     * the partition.
     */

    blocks = size / (64 * FWK_KIB);
    size = fwk_math_log2(blocks);

    /* Configure the base and the size of the remote memory region */
    ccg_ra_reg->CCG_RA_SAM_ADDR_REGION_REG[region_idx] = size | base;

    /* Configure the target HAID */
    ccg_ra_reg->CCG_RA_SAM_ADDR_REGION_REG[region_idx] |=
        ((uint64_t)target_haid << SAM_ADDR_TARGET_HAID_SHIFT);

    /* Mark the region as valid */
    ccg_ra_reg->CCG_RA_SAM_ADDR_REGION_REG[region_idx] |=
        SAM_ADDR_REG_VALID_MASK;
}

static void program_ra_sam(const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    unsigned int ccg_ldid;
    struct cmn_cyprus_ccg_ra_reg *ccg_ra_reg;
    uint8_t region_idx;
    uint8_t idx;

    idx = 0;
    do {
        /* Get the CCG RA logical ID */
        ccg_ldid = ccg_config->ldid[idx];
        ccg_ra_reg = shared_ctx->ccg_ra_reg_table[ccg_ldid].ccg_ra_reg;

        FWK_LOG_INFO(MOD_NAME "Programming CCG %u RA SAM...", ccg_ldid);

        for (region_idx = 0; region_idx < CMN_CYPRUS_MAX_RA_SAM_ADDR_REGION;
             region_idx++) {
            const struct mod_cmn_cyprus_ra_mem_region_map *ra_mmap =
                &ccg_config->ra_mmap_table[region_idx];

            ra_mmap = &ccg_config->ra_mmap_table[region_idx];

            /* If the size is zero, skip that entry */
            if (ra_mmap->size == 0) {
                continue;
            }

            FWK_LOG_INFO(
                MOD_NAME "  [0x%llx - 0x%llx] -> HAID %d",
                ra_mmap->base,
                (ra_mmap->base + (ra_mmap->size - 1)),
                ra_mmap->remote_haid[idx]);

            /* Configure the remote region in RA SAM register */
            configure_ra_sam_region(
                ccg_ra_reg,
                region_idx,
                ra_mmap->base,
                ra_mmap->size,
                ra_mmap->remote_haid[idx]);
        }
        FWK_LOG_INFO(MOD_NAME "Programming CCG %u RA SAM...Done", ccg_ldid);
        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

static void configure_linkid_reg(
    unsigned int ccg_ldid,
    uint8_t linkid,
    uint32_t agentid)
{
    uint32_t register_idx = 0;
    uint32_t agentid_bit_offset = 0;
    struct cmn_cyprus_ccg_ha_reg *ccg_ha_reg;
    struct cmn_cyprus_ccg_ra_reg *ccg_ra_reg;

    /* Get the CCG RA and HA registers */
    ccg_ra_reg = shared_ctx->ccg_ra_reg_table[ccg_ldid].ccg_ra_reg;
    ccg_ha_reg = shared_ctx->ccg_ha_reg_table[ccg_ldid].ccg_ha_reg;

    fwk_assert(agentid <= MAX_AGENT_ID);

    /* Each register is 64 bits and holds 8 AgentID/LinkID mappings */
    register_idx = agentid / 8;
    agentid_bit_offset = agentid % 8;

    /* Configure AgentID-to-LinkID */
    ccg_ra_reg->CCG_RA_AGENTID_TO_LINKID_REG[register_idx] |=
        ((uint64_t)linkid
         << (agentid_bit_offset * NUM_BITS_RESERVED_FOR_LINKID));

    ccg_ha_reg->CCG_HA_AGENTID_TO_LINKID_REG[register_idx] |=
        ((uint64_t)linkid
         << (agentid_bit_offset * NUM_BITS_RESERVED_FOR_LINKID));

    /* Mark the mapping as valid */
    ccg_ra_reg->CCG_RA_AGENTID_TO_LINKID_VAL |= (UINT64_C(0x1) << agentid);
    ccg_ha_reg->CCG_HA_AGENTID_TO_LINKID_VAL |= (UINT64_C(0x1) << agentid);
}

static void program_agentid_to_linkid_lut(
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    uint8_t linkid = 0;
    uint8_t remote_chip_id;
    uint8_t rnf_idx;
    uint8_t idx;
    uint16_t agent_id;
    uint8_t offset_id;
    uint8_t offset_pos;

    FWK_LOG_INFO(MOD_NAME "Program AgentID-to-LinkID LUT");

    remote_chip_id = ccg_config->remote_chip_id[linkid];
    offset_pos = fwk_math_log2(shared_ctx->config_table->chip_count);
    idx = 0;

    do {
        offset_id = (remote_chip_id * get_local_ra_count());
        /* Program the link IDs for remote agent IDs */
        for (rnf_idx = 0; rnf_idx <= shared_ctx->rnf_count; rnf_idx++) {
            agent_id = (remote_chip_id) | (offset_id << offset_pos);

            /* Extract [5:0] bits */
            agent_id = (agent_id & 0x3F);

            /* Program the linkID in the AgentID to LinkID register */
            configure_linkid_reg(ccg_config->ldid[idx], linkid, agent_id);

            /* Increment the offset ID to calculate the next agent id */
            offset_id++;
        }
        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

static void configure_ccg_ha_raid_to_ldid_reg(
    unsigned int ccg_ldid,
    uint8_t raid_id,
    uint8_t ldid_value)
{
    uint32_t register_idx = 0;
    uint32_t raid_bit_offset = 0;
    struct cmn_cyprus_ccg_ha_reg *ccg_ha_reg;

    ccg_ha_reg = shared_ctx->ccg_ha_reg_table[ccg_ldid].ccg_ha_reg;

    /* Each 64-bit RAID-to-LDID register holds 4 mappings, 16 bits each. */
    register_idx = raid_id / 4;
    raid_bit_offset = raid_id % 4;

    /* Write RAID-to-LDID mapping (with RNF bit set) */
    ccg_ha_reg->CCG_HA_RNF_EXP_RAID_TO_LDID_REG[register_idx] |=
        ((uint64_t)(
             ldid_value | CCG_HA_RAID_TO_LDID_RNF_MASK |
             (EXP_RAID_TO_LDID_VALID_MASK))
         << (raid_bit_offset * NUM_BITS_RESERVED_FOR_LDID));
}

/* Program the RAID-to-LDID LUT in CCG HA */
static void program_raid_to_ldid_lut(
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    unsigned int unique_remote_rnf_ldid_value;
    unsigned int local_ra_count;
    uint16_t raid;
    uint8_t offset_pos;
    uint8_t offset_id;
    uint8_t idx;

    /*
     * Get the local Request Agent count i.e., maximum of local request
     * node types (RN-Fs, RN-Is, RN-Ds).
     */
    local_ra_count = get_local_ra_count();

    if (ccg_config->remote_rnf_count && (shared_ctx->rnf_count == 0)) {
        FWK_LOG_ERR(
            MOD_NAME "Remote RN-F Count can't be %u when RN-F count is zero",
            ccg_config->remote_rnf_count);
        fwk_unexpected();
    }

    FWK_LOG_ERR(MOD_NAME "Program RAID-to-LDID LUT in CCG HA");

    idx = 0;
    do {
        /*
         * CCG HA must keep track of the remote RN-F nodes via unique
         * LDIDs in the local chip.
         *
         * Variable to keep track of the LDIDs assigned to the remote RN-F nodes.
         */
        unique_remote_rnf_ldid_value = shared_ctx->rnf_count;

        offset_pos = fwk_math_log2(shared_ctx->config_table->chip_count);

        for (uint8_t chip = 0; chip < shared_ctx->config_table->chip_count;
             chip++) {
            /* Skip local RN-Fs */
            if (chip == shared_ctx->chip_id) {
                continue;
            }
            offset_id = (chip * local_ra_count);

            /* Assign RAIDs for remote RN-Fs within the chip */
            for (unsigned int rnf_idx = 0; rnf_idx < shared_ctx->rnf_count;
                 rnf_idx++) {
                raid = chip | (offset_id << offset_pos);

                /* Program the CCHA RN-F RAID to LDID LUT */
                configure_ccg_ha_raid_to_ldid_reg(
                    ccg_config->ldid[idx], raid, unique_remote_rnf_ldid_value);

                offset_id++;
                unique_remote_rnf_ldid_value++;
            }
        }
        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

static void configure_hns_cpag(
    struct cmn_cyprus_hns_reg *hns,
    unsigned int ccg_ha_node_id,
    uint8_t cpag_id,
    uint8_t num_ports_in_cpag,
    uint8_t ccg_idx)
{
    uint8_t register_idx;
    uint8_t bit_pos = 0;

    /* Configure the target CCG node IDs within the CPAG */

    /* Calculate the register index */
    register_idx = ((cpag_id * num_ports_in_cpag) + ccg_idx) /
        CMN_PORT_AGGR_GRP_PAG_TGTID_PER_GROUP;

    /* Calculate the bit position */
    bit_pos =
        ((((cpag_id * num_ports_in_cpag) + ccg_idx) *
          CMN_PORT_AGGR_GRP_PAG_TGTID_WIDTH) %
         CMN_PORT_AGGR_GRP_PAG_TGTID_WIDTH_PER_GROUP);

    /* Only 0-1 CML_PORT_AGGR_GRP registers are defined */
    fwk_assert(register_idx < 2);

    /* Configure the target node ID */
    hns->CML_PORT_AGGR_GRP_REG[register_idx] |= ((uint64_t)ccg_ha_node_id)
        << bit_pos;
}

static void program_hns_ldid_to_chi_node_id(
    const struct mod_cmn_cyprus_ccg_config *ccg_config,
    unsigned int register_idx)
{
    unsigned int request_agent_nodeid;
    unsigned int hns_idx;
    unsigned int ccg_ha_ldid;
    unsigned int ccg_ha_node_id;
    uint8_t idx;
    struct cmn_cyprus_hns_reg *hns_reg;

    idx = 0;
    do {
        for (hns_idx = 0; hns_idx < shared_ctx->hns_count; hns_idx++) {
            hns_reg =
                (struct cmn_cyprus_hns_reg *)&shared_ctx->hns_table[hns_idx]
                    .hns;

            /* Skip isolated HN-S nodes */
            if (hns_reg == 0) {
                continue;
            }

            /* Assign the NodeID of CCHA as the remote Request node's NodeID */
            request_agent_nodeid =
                shared_ctx->ccg_ha_reg_table[ccg_config->ldid[idx]].node_id;

            /* Configure CCG HA NodeID */
            hns_reg->HNS_RN_CLUSTER_PHYSID[register_idx][0] |=
                (uint64_t)request_agent_nodeid;

            /* Configure the Request Node as remote  */
            hns_reg->HNS_RN_CLUSTER_PHYSID[register_idx][0] |=
                (uint64_t)REMOTE_CCG_NODE
                << HNS_RN_PHYS_RN_LOCAL_REMOTE_SHIFT_VAL;

            if (ccg_config->cpa_mode) {
                /* Enable CPA mode */
                hns_reg->HNS_RN_CLUSTER_PHYSID[register_idx][0] |=
                    ((uint64_t)(ccg_config->cpa_mode)
                     << HNF_RN_PHYS_CPA_EN_RA_SHIFT_VAL);

                /* Configure the CPAG ID */
                hns_reg->HNS_RN_CLUSTER_PHYSID[register_idx][0] |=
                    (ccg_config->cpag_id << HNF_RN_PHYS_CPA_GRP_RA_SHIFT_VAL);

                ccg_ha_ldid = ccg_config->ldid[idx];
                ccg_ha_node_id =
                    shared_ctx->ccg_ra_reg_table[ccg_ha_ldid].node_id;

                /* Configure the target CCG nodes in the CPAG */
                configure_hns_cpag(
                    hns_reg,
                    ccg_ha_node_id,
                    ccg_config->cpag_id,
                    ccg_config->num_ports_in_cpag,
                    idx);
            }

            /* Mark the mapping as valid */
            hns_reg->HNS_RN_CLUSTER_PHYSID[register_idx][0] |=
                (uint64_t)(UINT64_C(0x1) << HNS_RN_PHYS_RN_ID_VALID_SHIFT_VAL);
        }
        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

static void program_ccg_ha_id(
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    struct cmn_cyprus_ccg_ha_reg *ccg_ha_reg;
    unsigned int ccg_ldid;
    uint8_t idx;

    idx = 0;

    do {
        ccg_ldid = ccg_config->ldid[idx];
        ccg_ha_reg = shared_ctx->ccg_ha_reg_table[ccg_ldid].ccg_ha_reg;

        ccg_ha_reg->CCG_HA_ID = ccg_config->haid[idx];

        FWK_LOG_INFO(
            MOD_NAME "HAID for CCG %d (nodeid %d): HAID %d",
            ccg_ldid,
            get_node_id(ccg_ha_reg),
            ccg_config->haid[idx]);

        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

static void program_hns_ldid_to_rn_nodeid(
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    uint8_t linkid = 0;
    unsigned int remote_rnf_ldid_value;

    remote_rnf_ldid_value =
        (ccg_config->remote_chip_id[linkid] * shared_ctx->rnf_count);

    FWK_LOG_INFO(MOD_NAME "Programming remote request node IDs in HN-S");

    for (uint8_t i = 0; i < shared_ctx->rnf_count; i++) {
        program_hns_ldid_to_chi_node_id(ccg_config, remote_rnf_ldid_value);
        remote_rnf_ldid_value++;
    }
}

static void configure_ccg_ra_rnf_ldid_to_exp_raid_reg(
    uint8_t ccg_ldid,
    uint8_t rnf_ldid_value,
    uint16_t raid)
{
    uint32_t register_offset = 0;
    uint32_t ldid_value_offset = 0;
    struct cmn_cyprus_ccg_ra_reg *ccg_ra_reg;

    ccg_ra_reg = shared_ctx->ccg_ra_reg_table[ccg_ldid].ccg_ra_reg;

    /* Each 64-bit RA RNF LDID-to-RAID register holds 4 LDIDs */
    register_offset = rnf_ldid_value / 4;
    ldid_value_offset = rnf_ldid_value % 4;

    fwk_assert(register_offset < 128);

    /* Configure raid_value in LDID-to-RAID register */
    ccg_ra_reg->CCG_RA_RNF_LDID_TO_EXP_RAID_REG[register_offset] |=
        ((uint64_t)raid << (ldid_value_offset * NUM_BITS_RESERVED_FOR_RAID));

    /* Set corresponding valid bit */
    ccg_ra_reg->CCG_RA_RNF_LDID_TO_EXP_RAID_REG[register_offset] |=
        (LDID_TO_EXP_RAID_VALID_MASK)
        << (ldid_value_offset * NUM_BITS_RESERVED_FOR_RAID);
}

static void configure_ccg_ra_rnd_ldid_to_exp_raid_reg(
    uint8_t ccg_ldid,
    uint8_t ldid_value,
    uint16_t raid)
{
    uint32_t register_offset = 0;
    uint32_t ldid_value_offset = 0;
    struct cmn_cyprus_ccg_ra_reg *ccg_ra_reg;

    ccg_ra_reg = shared_ctx->ccg_ra_reg_table[ccg_ldid].ccg_ra_reg;

    /* Each 64-bit RA RND LDID-to-RAID register holds 4 LDIDs */
    register_offset = ldid_value / 4;
    ldid_value_offset = ldid_value % 4;

    fwk_assert(register_offset < 10);

    /* Configure raid_value in LDID-to-RAID register */
    ccg_ra_reg->CCG_RA_RND_LDID_TO_EXP_RAID_REG[register_offset] |=
        ((uint64_t)raid << (ldid_value_offset * NUM_BITS_RESERVED_FOR_RAID));

    /* Set corresponding valid bit */
    ccg_ra_reg->CCG_RA_RND_LDID_TO_EXP_RAID_REG[register_offset] |=
        (LDID_TO_EXP_RAID_VALID_MASK)
        << (ldid_value_offset * NUM_BITS_RESERVED_FOR_RAID);
}

static void configure_ccg_ra_rni_ldid_to_exp_raid_reg(
    uint8_t ccg_ldid,
    uint8_t ldid_value,
    uint16_t raid)
{
    uint32_t register_offset = 0;
    uint32_t ldid_value_offset = 0;
    struct cmn_cyprus_ccg_ra_reg *ccg_ra_reg;

    ccg_ra_reg = shared_ctx->ccg_ra_reg_table[ccg_ldid].ccg_ra_reg;

    /* Each 64-bit RA RND LDID-to-RAID register holds 4 LDIDs */
    register_offset = ldid_value / 4;
    ldid_value_offset = ldid_value % 4;

    fwk_assert(register_offset < 10);

    /* Configure raid_value in LDID-to-RAID register */
    ccg_ra_reg->CCG_RA_RNI_LDID_TO_EXP_RAID_REG[register_offset] |=
        ((uint64_t)raid << (ldid_value_offset * NUM_BITS_RESERVED_FOR_RAID));

    /* Set corresponding valid bit */
    ccg_ra_reg->CCG_RA_RNI_LDID_TO_EXP_RAID_REG[register_offset] |=
        (LDID_TO_EXP_RAID_VALID_MASK)
        << (ldid_value_offset * NUM_BITS_RESERVED_FOR_RAID);
}

static void program_ldid_to_raid_lut(
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    uint8_t rnf_ldid;
    uint8_t rni_ldid;
    uint8_t rnd_ldid;
    unsigned int local_ra_count;
    uint16_t raid;
    uint16_t offset_id;
    uint8_t offset_id_pos;
    uint8_t idx;

    /*
     * Get the local Request Agent count i.e., maximum of local request
     * node types (RN-Fs, RN-Is, RN-Ds).
     */
    local_ra_count = get_local_ra_count();

    /* Calculate the number of bits required to represent chip id */
    offset_id_pos = fwk_math_log2(shared_ctx->config_table->chip_count);

    idx = 0;
    do {
        /* Use chip ID for generating unique base IDs */
        offset_id = (shared_ctx->chip_id * local_ra_count);

        for (rnf_ldid = 0; rnf_ldid < shared_ctx->rnf_count; rnf_ldid++) {
            /* Generate unique Request Agent ID */
            raid = (shared_ctx->chip_id | (offset_id << offset_id_pos));

            /* Program RAID values in CCRA LDID to RAID LUT */
            configure_ccg_ra_rnf_ldid_to_exp_raid_reg(
                ccg_config->ldid[idx], rnf_ldid, raid);

            /* Increment the offset value sequentially */
            offset_id++;
        }

        /*
         * Reset the RAID value when programming for RN-D nodes
         * RAID values can overlap among the request nodes within a chip.
         */
        offset_id = (shared_ctx->chip_id * local_ra_count);

        for (rnd_ldid = 0; rnd_ldid < shared_ctx->rnd_count; rnd_ldid++) {
            /* Generate unique Request Agent ID */
            raid = (shared_ctx->chip_id | (offset_id << offset_id_pos));

            /* Program RAID values in CCRA LDID to RAID LUT */
            configure_ccg_ra_rnd_ldid_to_exp_raid_reg(
                ccg_config->ldid[idx], rnd_ldid, raid);

            /* Increment the offset value sequentially */
            offset_id++;
        }

        /*
         * Reset the RAID value when programming for RN-I nodes
         * RAID values can overlap among the request nodes within a chip.
         */
        offset_id = (shared_ctx->chip_id * local_ra_count);

        for (rni_ldid = 0; rni_ldid < shared_ctx->rni_count; rni_ldid++) {
            /* Generate unique Request Agent ID */
            raid = (shared_ctx->chip_id | (offset_id << offset_id_pos));

            /* Program RAID values in CCRA LDID to RAID LUT */
            configure_ccg_ra_rni_ldid_to_exp_raid_reg(
                ccg_config->ldid[idx], rni_ldid, raid);

            /* RAID value is assigned sequentially */
            offset_id++;
        }
        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

static void enable_smp_mode(const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    unsigned int ccg_ldid;
    uint8_t idx;
    struct cmn_cyprus_ccg_ra_reg *ccg_ra_reg;
    struct cmn_cyprus_ccg_ha_reg *ccg_ha_reg;

    idx = 0;
    do {
        ccg_ldid = ccg_config->ldid[idx];
        ccg_ra_reg = shared_ctx->ccg_ra_reg_table[ccg_ldid].ccg_ra_reg;
        ccg_ha_reg = shared_ctx->ccg_ha_reg_table[ccg_ldid].ccg_ha_reg;

        ccg_ra_reg->LINK_REGS[0].CCG_CCPRTCL_LINK_CTRL |=
            (1 << CCG_RA_CCPRTCL_LINK_CTRL_SMP_MODE_EN_SHIFT_VAL);
        ccg_ha_reg->LINK_REGS[0].CCG_CCPRTCL_LINK_CTRL |=
            (1 << CCG_HA_CCPRTCL_LINK_CTRL_SMP_MODE_EN_SHIFT_VAL);

        FWK_LOG_INFO(MOD_NAME "SMP Mode Enabled");
        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

/*
 * Helper function to check the status of the Upper link layer direct connect
 * (ull to ull) mode
 */
static bool check_ccla_ull_status(void *ccg_ldid)
{
    bool rx_ull_run_state;
    bool tx_ull_run_state;
    struct cmn_cyprus_ccla_reg *ccla_reg;

    ccla_reg = shared_ctx->ccla_reg_table[*(uint8_t *)ccg_ldid].ccla_reg;

    /* Read Rx ULL state */
    rx_ull_run_state =
        ((ccla_reg->CCLA_ULL_STATUS & CCLA_ULL_STATUS_SEND_RX_ULL_STATE_MASK) >
         0);

    /* Read Tx ULL state */
    tx_ull_run_state =
        ((ccla_reg->CCLA_ULL_STATUS & CCLA_ULL_STATUS_SEND_TX_ULL_STATE_MASK) >
         0);

    /* Check if both Rx ULL state and Tx ULL state bits are set to run state */
    return (rx_ull_run_state && tx_ull_run_state);
}

static void enable_ull_to_ull_mode(
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    int status;
    unsigned int ccg_ldid;
    uint8_t idx;
    struct cmn_cyprus_ccla_reg *ccla_reg;

    idx = 0;
    do {
        ccg_ldid = ccg_config->ldid[idx];
        ccla_reg = shared_ctx->ccla_reg_table[ccg_ldid].ccla_reg;

        FWK_LOG_INFO(MOD_NAME "Enabling ULL to ULL mode in CCG %u", ccg_ldid);

        /* Enable ULL-to-ULL mode */
        ccla_reg->CCLA_ULL_CTL =
            (1 << CCLA_ULL_CTL_ULL_TO_ULL_MODE_EN_SHIFT_VAL);

        /* Set send_vd_init */
        ccla_reg->CCLA_ULL_CTL |= (1 << CCLA_ULL_CTL_SEND_VD_INIT_SHIFT_VAL);

        /* Wait until link enable bits are set */
        status = shared_ctx->timer_api->wait(
            FWK_ID_ELEMENT(FWK_MODULE_IDX_TIMER, 0),
            CCLA_ULL_STATUS_TIMEOUT,
            check_ccla_ull_status,
            (void *)&ccg_ldid);

        if (status != FWK_SUCCESS) {
            if ((ccla_reg->CCLA_ULL_STATUS &
                 CCLA_ULL_STATUS_SEND_RX_ULL_STATE_MASK) == 0) {
                FWK_LOG_ERR(MOD_NAME "Rx ULL is in Stop state");
            }

            if ((ccla_reg->CCLA_ULL_STATUS &
                 CCLA_ULL_STATUS_SEND_TX_ULL_STATE_MASK) == 0) {
                FWK_LOG_ERR(MOD_NAME "Tx ULL is in Stop state");
            }

            FWK_LOG_ERR(
                MOD_NAME "Enabling ULL to ULL mode in CCG %u... Failed",
                ccg_ldid);
            fwk_trap();
        }

        FWK_LOG_INFO(
            MOD_NAME "Enabling ULL to ULL mode in CCG %u... Done", ccg_ldid);
        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

static bool ccg_link_wait_condition(void *data)
{
    bool ret;
    uint8_t linkid;
    uint64_t val1;
    uint64_t val2;
    unsigned int ccg_ldid;
    uint8_t idx;
    struct ccg_wait_condition_data *wait_data;
    struct cmn_cyprus_ccg_ra_reg *ccg_ra_reg;
    struct cmn_cyprus_ccg_ha_reg *ccg_ha_reg;

    fwk_assert(data != NULL);

    wait_data = (struct ccg_wait_condition_data *)data;

    idx = wait_data->idx;
    ccg_ldid = wait_data->ccg_config->ldid[idx];
    ccg_ra_reg = shared_ctx->ccg_ra_reg_table[ccg_ldid].ccg_ra_reg;
    ccg_ha_reg = shared_ctx->ccg_ha_reg_table[ccg_ldid].ccg_ha_reg;
    linkid = wait_data->linkid;

    switch (wait_data->cond) {
    case CCG_LINK_CTRL_EN_BIT_SET:
        /* Check if the link enable bits are set */
        val1 = ccg_ra_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_CTRL;
        val2 = ccg_ha_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_CTRL;
        ret = ((val1 & CCG_LINK_CTRL_EN_MASK) > 0) &&
            ((val2 & CCG_LINK_CTRL_EN_MASK) > 0);
        break;

    case CCG_LINK_CTRL_UP_BIT_CLR:
        /* Check if the link up bits are cleared */
        val1 = ccg_ra_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_CTRL;
        val2 = ccg_ha_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_CTRL;
        ret =
            (((val1 & CCG_LINK_CTRL_UP_MASK) == 0) &&
             ((val2 & CCG_LINK_CTRL_UP_MASK) == 0));
        break;

    case CCG_LINK_STATUS_DWN_BIT_SET:
        /* Check if the link down bits are set */
        val1 = ccg_ra_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_STATUS;
        val2 = ccg_ha_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_STATUS;
        ret = ((val1 & CCG_LINK_STATUS_DOWN_MASK) > 0) &&
            ((val2 & CCG_LINK_STATUS_DOWN_MASK) > 0);
        break;

    case CCG_LINK_STATUS_DWN_BIT_CLR:
        /* Check if link down bits are cleared */
        val1 = ccg_ra_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_STATUS;
        val2 = ccg_ha_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_STATUS;
        ret =
            (((val1 & CCG_LINK_STATUS_DOWN_MASK) == 0) &&
             ((val2 & CCG_LINK_STATUS_DOWN_MASK) == 0));
        break;

    case CCG_LINK_STATUS_ACK_BIT_SET:
        /* Check if link ACK bits are set */
        val1 = ccg_ra_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_STATUS;
        val2 = ccg_ha_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_STATUS;
        ret = ((val1 & CCG_LINK_STATUS_ACK_MASK) > 0) &&
            ((val2 & CCG_LINK_STATUS_ACK_MASK) > 0);
        break;

    case CCG_LINK_STATUS_ACK_BIT_CLR:
        /* Check if link ACK bits are clear */
        val1 = ccg_ra_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_STATUS;
        val2 = ccg_ha_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_STATUS;
        ret =
            (((val1 & CCG_LINK_STATUS_ACK_MASK) == 0) &&
             ((val2 & CCG_LINK_STATUS_ACK_MASK) == 0));
        break;

    case CCG_LINK_STATUS_HA_DVMDOMAIN_ACK_BIT_SET:
        /* Check if DVMDOMAIN ACK bit is set in CCG HA register */
        val1 = ccg_ha_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_STATUS;
        ret = (((val1 & CCG_LINK_STATUS_DVMDOMAIN_ACK_MASK)) != 0);
        break;

    case CCG_LINK_STATUS_RA_DVMDOMAIN_ACK_BIT_SET:
        /* Check if DVMDOMAIN ACK bit is set in CCG RA register */
        val1 = ccg_ra_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_STATUS;
        ret = (((val1 & CCG_LINK_STATUS_DVMDOMAIN_ACK_MASK)) != 0);
        break;

    default:
        fwk_unexpected();
        ret = false;
    }

    return ret;
}

static void enable_ccg_link(
    uint8_t linkid,
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    int status;
    unsigned int ccg_ldid;
    uint8_t idx;
    struct ccg_wait_condition_data wait_data;
    struct cmn_cyprus_ccg_ra_reg *ccg_ra_reg;
    struct cmn_cyprus_ccg_ha_reg *ccg_ha_reg;

    if (linkid > 2) {
        FWK_LOG_ERR(MOD_NAME "Error! LinkID must be in the range 0-2");
    }

    idx = 0;
    do {
        ccg_ldid = ccg_config->ldid[idx];
        ccg_ra_reg = shared_ctx->ccg_ra_reg_table[ccg_ldid].ccg_ra_reg;
        ccg_ha_reg = shared_ctx->ccg_ha_reg_table[ccg_ldid].ccg_ha_reg;

        FWK_LOG_INFO(MOD_NAME "Enabling CCG %u link %d...", ccg_ldid, linkid);

        /* Set link enable bit to enable the CCG link */
        ccg_ra_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_CTRL |=
            CCG_LINK_CTRL_EN_MASK;
        ccg_ha_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_CTRL |=
            CCG_LINK_CTRL_EN_MASK;

        /* Configure the wait condition */
        wait_data.ccg_config = ccg_config;
        wait_data.idx = idx;
        wait_data.linkid = linkid;
        wait_data.cond = CCG_LINK_CTRL_EN_BIT_SET;

        /* Wait until link enable bits are set */
        status = shared_ctx->timer_api->wait(
            FWK_ID_ELEMENT(FWK_MODULE_IDX_TIMER, 0),
            CCG_CCPRTCL_LINK_CTRL_TIMEOUT,
            ccg_link_wait_condition,
            &wait_data);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(
                MOD_NAME "Enabling CCG %u link %d... Failed", ccg_ldid, linkid);
            fwk_trap();
        }

        FWK_LOG_INFO(
            MOD_NAME "Enabling CCG %u link %d... Done", ccg_ldid, linkid);
        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

static void verify_ccg_link_is_down(
    uint8_t linkid,
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    int status;
    uint8_t idx;
    struct ccg_wait_condition_data wait_data;

    idx = 0;

    do {
        FWK_LOG_INFO(
            MOD_NAME "Verifying CCG %u link %u is down...",
            ccg_config->ldid[idx],
            linkid);

        /* Configure the wait condition */
        wait_data.ccg_config = ccg_config;
        wait_data.idx = idx;
        wait_data.linkid = linkid;
        wait_data.cond = CCG_LINK_CTRL_UP_BIT_CLR;

        /* Wait till link up bits are cleared in control register */
        status = shared_ctx->timer_api->wait(
            FWK_ID_ELEMENT(FWK_MODULE_IDX_TIMER, 0),
            CCG_CCPRTCL_LINK_CTRL_TIMEOUT,
            ccg_link_wait_condition,
            &wait_data);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(MOD_NAME "Error! Link up bits are not cleared");
            FWK_LOG_ERR(
                MOD_NAME "Verifying CCG %u link %u down status... Failed",
                ccg_config->ldid[idx],
                linkid);
            fwk_trap();
        }

        /* Wait till link down bits are set in status register */
        wait_data.cond = CCG_LINK_STATUS_DWN_BIT_SET;
        status = shared_ctx->timer_api->wait(
            FWK_ID_ELEMENT(FWK_MODULE_IDX_TIMER, 0),
            CCG_CCPRTCL_LINK_CTRL_TIMEOUT,
            ccg_link_wait_condition,
            &wait_data);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(MOD_NAME "Error! Link down bits are not set");
            FWK_LOG_ERR(
                MOD_NAME "Verifying CCG %u link %u down status... Failed",
                ccg_config->ldid[idx],
                linkid);
            fwk_trap();
        }

        /* Wait till link ACK bits are cleared in status register */
        wait_data.cond = CCG_LINK_STATUS_ACK_BIT_CLR;
        status = shared_ctx->timer_api->wait(
            FWK_ID_ELEMENT(FWK_MODULE_IDX_TIMER, 0),
            CCG_CCPRTCL_LINK_CTRL_TIMEOUT,
            ccg_link_wait_condition,
            &wait_data);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(MOD_NAME "Error! Link ack bits are not cleared");
            FWK_LOG_ERR(
                MOD_NAME "Verifying CCG %u link %u down status... Failed",
                ccg_config->ldid[idx],
                linkid);
            fwk_trap();
        }

        FWK_LOG_ERR(
            MOD_NAME "Verifying CCG %u link %u down status... Done",
            ccg_config->ldid[idx],
            linkid);
        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

static void bring_up_ccg_link(
    uint8_t linkid,
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    unsigned int ccg_ldid;
    uint8_t idx;
    struct ccg_wait_condition_data wait_data;
    struct cmn_cyprus_ccg_ra_reg *ccg_ra_reg;
    struct cmn_cyprus_ccg_ha_reg *ccg_ha_reg;
    int status;

    idx = 0;

    do {
        ccg_ldid = ccg_config->ldid[idx];
        ccg_ra_reg = shared_ctx->ccg_ra_reg_table[ccg_ldid].ccg_ra_reg;
        ccg_ha_reg = shared_ctx->ccg_ha_reg_table[ccg_ldid].ccg_ha_reg;

        FWK_LOG_INFO(
            MOD_NAME "Bringing up CCG %u link %u...", ccg_ldid, linkid);

        /* Bring up link using link request bit */
        ccg_ra_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_CTRL |=
            CCG_LINK_CTRL_REQ_MASK;
        ccg_ha_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_CTRL |=
            CCG_LINK_CTRL_REQ_MASK;

        /* Configure the wait condition */
        wait_data.ccg_config = ccg_config;
        wait_data.idx = idx;
        wait_data.linkid = linkid;
        wait_data.cond = CCG_LINK_STATUS_ACK_BIT_SET;

        /* Wait till link ACK bits are set in status register */
        status = shared_ctx->timer_api->wait(
            FWK_ID_ELEMENT(FWK_MODULE_IDX_TIMER, 0),
            CCG_CCPRTCL_LINK_CTRL_TIMEOUT,
            ccg_link_wait_condition,
            &wait_data);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(MOD_NAME "Error! Link ack bits are not set");
            FWK_LOG_ERR(
                MOD_NAME "Bringing up CCG %u link %u... Failed",
                ccg_ldid,
                linkid);
            fwk_trap();
        }

        /* Wait till link down bits are cleared in status register */
        wait_data.cond = CCG_LINK_STATUS_DWN_BIT_CLR;
        status = shared_ctx->timer_api->wait(
            FWK_ID_ELEMENT(FWK_MODULE_IDX_TIMER, 0),
            CCG_CCPRTCL_LINK_CTRL_TIMEOUT,
            ccg_link_wait_condition,
            &wait_data);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(MOD_NAME "Error! Link down bits are not cleared");
            FWK_LOG_ERR(
                MOD_NAME "Bringing up CCG %u link %u... Failed",
                ccg_ldid,
                linkid);
            fwk_trap();
        }

        FWK_LOG_INFO(
            MOD_NAME "Bringing up CCG %u link %u... Done", ccg_ldid, linkid);
        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

static void ccg_exchange_protocol_credit(
    uint8_t linkid,
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    unsigned int ccg_ldid;
    uint8_t idx;
    struct cmn_cyprus_ccg_ra_reg *ccg_ra_reg;
    struct cmn_cyprus_ccg_ha_reg *ccg_ha_reg;

    idx = 0;
    do {
        ccg_ldid = ccg_config->ldid[idx];
        ccg_ra_reg = shared_ctx->ccg_ra_reg_table[ccg_ldid].ccg_ra_reg;
        ccg_ha_reg = shared_ctx->ccg_ha_reg_table[ccg_ldid].ccg_ha_reg;

        FWK_LOG_INFO(
            MOD_NAME "Exchanging protocol credits for CCG %u link %d...",
            ccg_ldid,
            linkid);

        /* Exchange protocol credits using link up bit */
        ccg_ra_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_CTRL |=
            CCG_LINK_CTRL_UP_MASK;
        ccg_ha_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_CTRL |=
            CCG_LINK_CTRL_UP_MASK;

        FWK_LOG_INFO(
            MOD_NAME "Exchanging protocol credits for CCG %u link %d... Done",
            ccg_ldid,
            linkid);
        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

static void ccg_enter_system_coherency(
    uint8_t linkid,
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    int status;
    unsigned int ccg_ldid;
    uint8_t idx;
    struct ccg_wait_condition_data wait_data;
    struct cmn_cyprus_ccg_ha_reg *ccg_ha_reg;

    idx = 0;
    do {
        ccg_ldid = ccg_config->ldid[idx];
        ccg_ha_reg = shared_ctx->ccg_ha_reg_table[ccg_ldid].ccg_ha_reg;

        FWK_LOG_INFO(
            MOD_NAME "Entering system coherency for CCG %u link %d...",
            ccg_ldid,
            linkid);

        /* Enter system coherency by setting DVMDOMAIN request bit */
        ccg_ha_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_CTRL |=
            CCG_LINK_CTRL_DVMDOMAIN_REQ_MASK;

        /* Configure the wait condition */
        wait_data.linkid = linkid;
        wait_data.idx = idx;
        wait_data.ccg_config = ccg_config;
        wait_data.cond = CCG_LINK_STATUS_HA_DVMDOMAIN_ACK_BIT_SET;

        /* Wait till DVMDOMAIN ACK bit is set in status register */
        status = shared_ctx->timer_api->wait(
            FWK_ID_ELEMENT(FWK_MODULE_IDX_TIMER, 0),
            CCG_CCPRTCL_LINK_DVMDOMAIN_TIMEOUT,
            ccg_link_wait_condition,
            &wait_data);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(
                MOD_NAME
                "Entering system coherency for CCG %u link %d... Failed",
                ccg_ldid,
                linkid);
            fwk_trap();
        }

        FWK_LOG_INFO(
            MOD_NAME "Entering system coherency for CCG %u link %d... Done",
            ccg_ldid,
            linkid);
        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

static void ccg_enter_dvm_domain(
    uint8_t linkid,
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    int status;
    unsigned int ccg_ldid;
    uint8_t idx;
    struct ccg_wait_condition_data wait_data;
    struct cmn_cyprus_ccg_ra_reg *ccg_ra_reg;

    idx = 0;
    do {
        ccg_ldid = ccg_config->ldid[idx];
        wait_data.ccg_config = ccg_config;
        ccg_ra_reg = shared_ctx->ccg_ra_reg_table[ccg_ldid].ccg_ra_reg;

        FWK_LOG_INFO(
            MOD_NAME "Entering DVM domain for CCG %u link %d...",
            ccg_ldid,
            linkid);

        /* DVM domain entry by setting DVMDOMAIN request bit */
        ccg_ra_reg->LINK_REGS[linkid].CCG_CCPRTCL_LINK_CTRL |=
            CCG_LINK_CTRL_DVMDOMAIN_REQ_MASK;

        /* Configure wait data */
        wait_data.linkid = linkid;
        wait_data.idx = idx;
        wait_data.cond = CCG_LINK_STATUS_RA_DVMDOMAIN_ACK_BIT_SET;

        /* Wait till DVMDOMAIN ACK bit is set in status register */
        status = shared_ctx->timer_api->wait(
            FWK_ID_ELEMENT(FWK_MODULE_IDX_TIMER, 0),
            CCG_CCPRTCL_LINK_DVMDOMAIN_TIMEOUT,
            ccg_link_wait_condition,
            &wait_data);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(
                MOD_NAME "Entering DVM domain for CCG %u link %d... Failed",
                ccg_ldid,
                linkid);
            fwk_trap();
        }

        FWK_LOG_INFO(
            MOD_NAME "Entering DVM domain for CCG %u link %d... Done",
            ccg_ldid,
            linkid);

        idx++;
    } while (idx < ccg_config->num_ports_in_cpag);
}

void bring_up_cml_protocol_links(
    uint8_t linkid,
    const struct mod_cmn_cyprus_ccg_config *ccg_config)
{
    enable_ccg_link(linkid, ccg_config);

    verify_ccg_link_is_down(linkid, ccg_config);

    bring_up_ccg_link(linkid, ccg_config);
}

static void setup_cml_protocol_links(void)
{
    const struct mod_cmn_cyprus_ccg_config *ccg_config;
    unsigned int ccg_idx;
    uint8_t linkid;

    linkid = 0;

    for (ccg_idx = 0; ccg_idx < shared_ctx->config->ccg_table_count;
         ccg_idx++) {
        ccg_config = &shared_ctx->config->ccg_config_table[ccg_idx];

        /* Exchange protocol credits with remote CML link */
        ccg_exchange_protocol_credit(linkid, ccg_config);

        /* Enable the CML links to enter system coherency domain */
        ccg_enter_system_coherency(linkid, ccg_config);

        /* Enable the CML links to enter DVM domain */
        ccg_enter_dvm_domain(linkid, ccg_config);

        FWK_LOG_INFO(
            MOD_NAME "Chip %u to Chip %u CML configured",
            shared_ctx->chip_id,
            ccg_config->remote_chip_id[0]);
    }
}

void cmn_cyprus_setup_cml(struct cmn_cyprus_ctx *ctx)
{
    unsigned int ccg_idx;
    uint8_t linkid;
    const struct mod_cmn_cyprus_ccg_config *ccg_config;

    /* Initialize the shared context pointer */
    shared_ctx = ctx;

    /*
     * Initialize the remote RN-F LDID value. The LDID of remote RN-F nodes need
     * to be programmed in the HN-S. The LDID of the remote RN-F nodes begin
     * from local chip's last RN-F LDID value + 1.
     */
    shared_ctx->remote_rnf_ldid_value = shared_ctx->rnf_count;

    /* Iterate through each CCG configuration and program CCG */
    for (ccg_idx = 0; ccg_idx < shared_ctx->config->ccg_table_count;
         ccg_idx++) {
        ccg_config = &shared_ctx->config->ccg_config_table[ccg_idx];

        /* Program the RA SAM */
        program_ra_sam(ccg_config);

        /* Assign LinkIDs to remote CML protocol links */
        program_agentid_to_linkid_lut(ccg_config);

        /* Program the HAID in the CCG HA node */
        program_ccg_ha_id(ccg_config);

        /* Assign LDIDs to remote caching agents in CCG HA node */
        program_raid_to_ldid_lut(ccg_config);

        /*
         * Program the HA NodeID at the LDID index of each remote RN‑F
         * in HN-S nodes.
         */
        program_hns_ldid_to_rn_nodeid(ccg_config);

        /*
         * Program unique RAID for each Request Node in CCG RA
         * LDID-to-RAID LUT.
         */
        program_ldid_to_raid_lut(ccg_config);

        if (ccg_config->smp_mode) {
            enable_smp_mode(ccg_config);
        }

        if (ccg_config->ull_to_ull_mode) {
            enable_ull_to_ull_mode(ccg_config);
        }

        /* Only link 0 is supported at the moment */
        linkid = 0;

        /* Establish protocol between local CCG and remote CCG */
        bring_up_cml_protocol_links(linkid, ccg_config);
    }

    /*
     * Enable the CML protocol links to exchange protocol credits,
     * enter system coherency domain and DVM domain.
     *
     * Note: Only Link 0 is supported at the moment.
     */
    setup_cml_protocol_links();

    FWK_LOG_INFO(MOD_NAME "CML setup complete");
}
