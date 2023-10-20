/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions and utility functions for the programming HN-SAM.
 */

#include <internal/cmn_cyprus_ctx.h>
#include <internal/cmn_cyprus_hnsam.h>
#include <internal/cmn_cyprus_reg.h>
#include <internal/cmn_cyprus_utils.h>

#include <mod_cmn_cyprus.h>

#include <fwk_assert.h>
#include <fwk_log.h>
#include <fwk_status.h>

#include <inttypes.h>
#include <stdint.h>

/* HN-S SAM */
#define CMN_CYPRUS_HNS_UNIT_INFO_HNSAM_RCOMP_EN_MASK (0x10000000)
#define CMN_CYPRUS_HNS_UNIT_INFO_HNSAM_RCOMP_EN_POS  28
#define CMN_CYPRUS_HNS_SAM_MEMREGION_SIZE_POS        12
#define CMN_CYPRUS_HNS_SAM_MEMREGION_BASE_POS        20
#define CMN_CYPRUS_HNS_SAM_MEMREGION_VALID           UINT64_C(0x8000000000000000)

/* HN-S SAM_CONTROL */
#define CMN_CYPRUS_HNS_SAM_CONTROL_SN_MODE_POS(sn_mode)   (36 + sn_mode - 1)
#define CMN_CYPRUS_HNS_SAM_CONTROL_TOP_ADDR_BIT0_POS      40
#define CMN_CYPRUS_HNS_SAM_CONTROL_TOP_ADDR_BIT1_POS      48
#define CMN_CYPRUS_HNS_SAM_CONTROL_SN_NODE_ID_POS(sn_idx) (sn_idx * 12)

/* HN-S Power Policy */
#define CMN_CYPRUS_PPU_PWPR_POLICY_ON  UINT64_C(0x0000000000000008)
#define CMN_CYPRUS_PPU_PWPR_OPMODE_FAM UINT64_C(0x0000000000000030)
#define CMN_CYPRUS_PPU_PWPR_DYN_EN     UINT64_C(0x0000000000000100)

/* Shared driver context pointer */
static struct cmn_cyprus_ctx *shared_ctx;

/*
 * Configure Hierarchichal Hashing in HN-F SAM.
 */
static void setup_hierarchichal_hashing(
    struct cmn_cyprus_hns_reg *hns,
    unsigned int logical_id,
    const struct mod_cmn_cyprus_hierarchical_hashing *hier_hash_cfg)
{
    unsigned int hns_count_per_cluster;
    unsigned int hns_cluster_index;
    uint64_t sam_control_value;
    unsigned int snf_count_per_cluster;
    unsigned int snf_idx_in_cluster;
    unsigned int snf_table_idx;
    enum mod_cmn_cyprus_hns_to_snf_mem_strip_mode sn_mode;
    unsigned int top_address_bit0, top_address_bit1;

    sn_mode = hier_hash_cfg->sn_mode;
    top_address_bit0 = hier_hash_cfg->top_address_bit0;
    top_address_bit1 = hier_hash_cfg->top_address_bit1;

    /* Number of SN-F nodes in a cluster */
    snf_count_per_cluster =
        shared_ctx->config->snf_count / hier_hash_cfg->hns_cluster_count;

    /*
     * Only 3-SN mode (3 SN-F nodes per cluster) is supported currently.
     */
    fwk_assert(snf_count_per_cluster == 3);

    /* Number of HN-S nodes in a cluster */
    hns_count_per_cluster =
        shared_ctx->hns_count / hier_hash_cfg->hns_cluster_count;

    if (top_address_bit1 <= top_address_bit0) {
        FWK_LOG_ERR(
            MOD_NAME
            "top_address_bit1: %d should be greater than top_address_bit0: "
            "%d",
            top_address_bit1,
            top_address_bit0);
        fwk_unexpected();
    }

    /* Choose the cluster idx based on the HN-S LDID value */
    hns_cluster_index = logical_id / hns_count_per_cluster;

    /* Configure SN mode (3-SN, 5-SN or 6-SN)*/
    sam_control_value =
        (UINT64_C(1) << CMN_CYPRUS_HNS_SAM_CONTROL_SN_MODE_POS(sn_mode));

    /* Configure bit position of top_address_bit0 */
    sam_control_value |=
        ((uint64_t)top_address_bit0
         << CMN_CYPRUS_HNS_SAM_CONTROL_TOP_ADDR_BIT0_POS);

    /* Configure bit position of top_address_bit1 */
    sam_control_value |=
        ((uint64_t)top_address_bit1
         << CMN_CYPRUS_HNS_SAM_CONTROL_TOP_ADDR_BIT1_POS);

    /* Iterate through indices of SN-F nodes present within the cluster */
    for (snf_idx_in_cluster = 0; snf_idx_in_cluster < snf_count_per_cluster;
         snf_idx_in_cluster++) {
        snf_table_idx =
            (hns_cluster_index * snf_count_per_cluster) + snf_idx_in_cluster;

        /* Configure the SN node ID */
        sam_control_value |=
            ((uint64_t)shared_ctx->config->snf_table[snf_table_idx]
             << CMN_CYPRUS_HNS_SAM_CONTROL_SN_NODE_ID_POS(snf_idx_in_cluster));
    }

    /* Configure the HN-F SAM */
    hns->SAM_CONTROL = sam_control_value;
}

/*
 * Program the SN0 target ID. HN-F is directly mapped to an SN-F node.
 */
static void configure_default_hashed_region(
    struct cmn_cyprus_hns_reg *hns,
    unsigned int snf_node_id)
{
    /* Set target node */
    hns->SAM_CONTROL = snf_node_id;
}

static bool get_hnsam_range_comp_en_mode(void *hns_reg)
{
    struct cmn_cyprus_hns_reg *hns = hns_reg;
    return (hns->UNIT_INFO[1] & CMN_CYPRUS_HNS_UNIT_INFO_HNSAM_RCOMP_EN_MASK) >>
        CMN_CYPRUS_HNS_UNIT_INFO_HNSAM_RCOMP_EN_POS;
}

/*
 * Configure range-based SN-F mapping for syscache sub memory region.
 */
static void map_syscache_sub_region(
    struct cmn_cyprus_hns_reg *hns,
    const struct mod_cmn_cyprus_mem_region_map *region,
    unsigned int region_idx)
{
    uint64_t base;

    /* Offset the base with chip address space base on chip-id */
    base =
        ((uint64_t)(shared_ctx->config->chip_addr_space * shared_ctx->chip_id) +
         region->base);

    /* Only 2 range-based memory regions can be configured */
    fwk_assert(region_idx < 2);

    /* Configure sub-region entry */
    if (get_hnsam_range_comp_en_mode(hns)) {
        /* Configure end address of the region */
        hns->SAM_MEMREGION_END_ADDR[region_idx] = ((base + region->size - 1));

        /* Configure base address of the region */
        hns->SAM_MEMREGION[region_idx] |=
            ((base / SAM_GRANULARITY) << CMN_CYPRUS_HNS_SAM_MEMREGION_BASE_POS);
    } else {
        /* Configure region size */
        hns->SAM_MEMREGION[region_idx] |=
            (sam_encode_region_size(region->size)
             << CMN_CYPRUS_HNS_SAM_MEMREGION_SIZE_POS);

        /* Configure region base */
        hns->SAM_MEMREGION[region_idx] |=
            ((base / SAM_GRANULARITY) << CMN_CYPRUS_HNS_SAM_MEMREGION_BASE_POS);
    }

    /* Configure target node ID */
    hns->SAM_MEMREGION[region_idx] |= region->node_id;

    /* Set the region as valid */
    hns->SAM_MEMREGION[region_idx] |= CMN_CYPRUS_HNS_SAM_MEMREGION_VALID;
}

static void configure_syscache_sub_regions(struct cmn_cyprus_hns_reg *hns)
{
    const struct mod_cmn_cyprus_config *config = shared_ctx->config;
    unsigned int region_idx;
    unsigned int syscache_sub_region_count = 0;

    /*
     * Map syscache sub-regions to this HN-S node.
     */
    for (region_idx = 0; region_idx < config->mmap_count; region_idx++) {
        const struct mod_cmn_cyprus_mem_region_map *region =
            &config->mmap_table[region_idx];

        /* Skip non-syscache sub-regions */
        if (region->type != MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE_SUB) {
            continue;
        }

        map_syscache_sub_region(hns, region, syscache_sub_region_count);
        syscache_sub_region_count++;
    }
}

static void configure_hns_pwpr(struct cmn_cyprus_hns_reg *hns)
{
    /* Configure the system cache RAM PPU */
    hns->PPU_PWPR = CMN_CYPRUS_PPU_PWPR_POLICY_ON |
        CMN_CYPRUS_PPU_PWPR_OPMODE_FAM | CMN_CYPRUS_PPU_PWPR_DYN_EN;
}

void cmn_cyprus_setup_hnf_sam(struct cmn_cyprus_ctx *ctx)
{
    const struct mod_cmn_cyprus_config *config;
    const struct mod_cmn_cyprus_hierarchical_hashing *hier_hash_cfg;
    struct cmn_cyprus_hns_reg *hns;
    unsigned int hns_idx;
    unsigned int logical_id;

    /* Initialize the shared context pointer */
    shared_ctx = ctx;
    config = shared_ctx->config;
    hier_hash_cfg = &(config->hierarchical_hashing_config);

    /* Iterate through each HN-S node and configure the HN-F SAM */
    for (hns_idx = 0; hns_idx < shared_ctx->hns_count; hns_idx++) {
        hns = (struct cmn_cyprus_hns_reg *)shared_ctx->hns_table[hns_idx].hns;

        /* Skip isolated HN-S nodes */
        if (hns == 0) {
            continue;
        }

        logical_id = get_node_logical_id(hns);

        if (config->hierarchical_hashing_enable && hier_hash_cfg->sn_mode) {
            /* Setup Hierarchical Hashing in HN-S nodes */
            setup_hierarchichal_hashing(hns, logical_id, hier_hash_cfg);
        } else {
            /* Incorrect SN-F table configuration */
            fwk_assert(logical_id < config->snf_count);

            /* Default Hashed region: Direct mapping */
            configure_default_hashed_region(hns, config->snf_table[logical_id]);
        }

        /* Map syscache sub-regions to this HN-S node */
        configure_syscache_sub_regions(hns);

        /* Configure the power policy */
        configure_hns_pwpr(hns);
    }

    FWK_LOG_INFO(MOD_NAME "HN-F SAM setup complete");
}
