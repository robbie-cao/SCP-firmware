/*
 * Copyright (c) 2023-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <tower_nci_reg.h>

#include <mod_clock.h>
#include <mod_tower_nci.h>

#include <fwk_assert.h>
#include <fwk_event.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_notification.h>
#include <fwk_status.h>

#include <stdio.h>
#include <stdlib.h>

#define TOWER_NCI_ADDRESS_PAGE_SIZE (1ULL << 12)
#define TOWER_NCI_ADDRESS(addr) (addr & ~(TOWER_NCI_ADDRESS_PAGE_SIZE - 1))
#define TOWER_NCI_TARGET_ID_SIZE (1ULL << 8)
#define TOWER_NCI_TARGET_ID(target_id) (target_id & \
        (TOWER_NCI_TARGET_ID_SIZE - 1))
struct mod_tower_nci_element_ctx {
    struct tower_nci_config *config;
};

struct mod_tower_nci_ctx {
    struct mod_tower_nci_element_ctx *element_ctx;
    unsigned int element_count;
};

struct mod_tower_nci_ctx tower_nci_ctx;

#if FWK_LOG_LEVEL <= FWK_LOG_LEVEL_INFO
static char *node_type_str[TOWER_NCI_NODE_TYPE_MAX] = {
    "CFGNI",
    "VD",
    "PD",
    "CD",
    "ASNI",
    "AMNI",
    "PMU",
    "HSNI",
    "HMNI",
    "PMNI",
};
#endif

/*
 * PSAM Programming
 */
static int tower_nci_psam_nhregion_init(
                        uintptr_t base, uint64_t base_addr,
                        uint64_t end_addr, uint64_t target_id, uint64_t region)
{
    struct tower_nci_psam_reg_map* reg =
        (struct tower_nci_psam_reg_map*)base;
    if (reg == NULL) {
        return FWK_E_PARAM;
    }

    FWK_LOG_INFO("Programming Region: %lld region at: 0x%x", region, base);
    FWK_LOG_INFO("Address: Start: 0x%llx, End: 0x%llx", base_addr, end_addr);
    FWK_LOG_INFO("Target: 0x%llx", target_id);

    // Disable region.
    reg->nh_region[region].cfg0 &= ~0x1;
    __sync_synchronize();
    // Set base address
    reg->nh_region[region].cfg1_cfg0 = TOWER_NCI_ADDRESS(base_addr);
    // Set end address
    reg->nh_region[region].cfg3_cfg2 =
        TOWER_NCI_ADDRESS(end_addr) | TOWER_NCI_TARGET_ID(target_id);

    __sync_synchronize();
    // Set region valid.
    reg->nh_region[region].cfg0 |= 1;


    return FWK_SUCCESS;
}

static int tower_nci_find_region_in_psam(uintptr_t base, uint64_t base_addr,
                                         uint64_t target_id, uint64_t *region)
{
    struct tower_nci_psam_reg_map* reg;
    uint32_t count;

    reg = (struct tower_nci_psam_reg_map *)base;
    if ((reg == NULL) || (region == NULL)) {
        return FWK_E_PARAM;
    }

    for (count = 0; count < TOWER_NCI_MAX_NUM_REGIONS; count++) {
            if (((TOWER_NCI_ADDRESS(reg->nh_region[count].cfg1_cfg0) ==
                        TOWER_NCI_ADDRESS(base_addr)) &&
                (TOWER_NCI_TARGET_ID(reg->nh_region[count].cfg3_cfg2) ==
                 TOWER_NCI_TARGET_ID(target_id))) ||
                ((reg->nh_region[count].cfg0 & 1) == 0)) {
                *region = count;
                return FWK_SUCCESS;
            }
    }

    return FWK_E_RANGE;
}

static int tower_nci_psam_enable(uintptr_t base)
{
    struct tower_nci_psam_reg_map *reg = (struct tower_nci_psam_reg_map *)base;

    if (reg == NULL) {
        return FWK_E_PARAM;
    }

    reg->sam_status = 0x1;

    return FWK_SUCCESS;
}

static int tower_nci_psam_disable(uintptr_t base)
{
    struct tower_nci_psam_reg_map* reg = (struct tower_nci_psam_reg_map *)base;

    if (reg == NULL) {
        return FWK_E_PARAM;
    }

    reg->sam_status = 0x0;

    return FWK_SUCCESS;
}

static int tower_nci_program_psam_regions(uint32_t psam_base,
        struct tower_nci_psam_region *psam_regions, uint32_t count)
{
    uint8_t region_idx;
    int status;

    status = tower_nci_psam_disable(psam_base);
    if (status != FWK_SUCCESS) {
        return status;
    }

    for (region_idx = 0; region_idx < count; region_idx++) {
        status = tower_nci_psam_nhregion_init(psam_base,
                psam_regions[region_idx].base_address,
                (psam_regions[region_idx].base_address +
                    psam_regions[region_idx].size) - 1,
                psam_regions[region_idx].node_id, region_idx);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return tower_nci_psam_enable(psam_base);
}

uintptr_t mod_tower_nci_process_subfeatures(uintptr_t base, uintptr_t offset,
                                            uint16_t sub_feature_type)
{
    struct tower_nci_component_cfg_hdr *cfg_hdr;
    uint32_t count;
    uint32_t num_sub_features;
    uint32_t node_type;

    cfg_hdr = (struct tower_nci_component_cfg_hdr *)(base + offset);
    num_sub_features = cfg_hdr->num_sub_features;
    for(count = 0; count < num_sub_features; count++) {
        node_type = cfg_hdr->sub_feature[count].node_type & 0xFF;
        if (node_type == sub_feature_type) {
            return base + cfg_hdr->sub_feature[count].pointer;
        }
    }
    FWK_LOG_ERR("Subfeature not found: %d", sub_feature_type);
    return 0;
}

uint32_t mod_tower_nci_get_subfeature_address(
        uintptr_t base, uintptr_t offset,
        enum tower_nci_node_type node_type,
        uint16_t id, enum mod_tower_nci_subfeatue_type sub_feature_type)
{
    struct tower_nci_domain_cfg_hdr *cfg_hdr;
    uintptr_t subfeature_address;
    uint32_t child_count;
    uint32_t idx;
    uint32_t hdr_type;
    uint16_t hdr_id;

    cfg_hdr = (struct tower_nci_domain_cfg_hdr *)(base + offset);
    hdr_type = cfg_hdr->node_type;
    hdr_id = (hdr_type >> 16) & 0xFF;
    hdr_type &= 0xFF;
    if ((hdr_type == node_type) && (hdr_id == id)) {
        if (node_type < TOWER_NCI_NODE_TYPE_ASNI) {
            FWK_LOG_ERR("Invalid node type: %d", node_type);
            return 0;
        }
        FWK_LOG_INFO("Found Node: %s ID: %d", node_type_str[hdr_type],
                     hdr_id);
        return mod_tower_nci_process_subfeatures(base, offset,
                                                 sub_feature_type);
    }

    subfeature_address = 0;
    if (hdr_type < TOWER_NCI_NODE_TYPE_ASNI) {
        child_count = cfg_hdr->child_node_info;
        for (idx = 0; idx < child_count; idx++) {
            subfeature_address =
                mod_tower_nci_get_subfeature_address(
                        base, cfg_hdr->x_pointers[idx], node_type, id,
                        sub_feature_type);
            if (subfeature_address != 0) {
                break;
            }
        }
    }
    return subfeature_address;
}

static int mod_tower_nci_init(
        fwk_id_t module_id,
        unsigned int block_count,
        const void *unused)
{
    if (block_count == 0) {
        /* Configuration will be done during runtime. */
        return FWK_SUCCESS;
    }

    tower_nci_ctx.element_ctx = fwk_mm_calloc(block_count,
                                  sizeof(struct mod_tower_nci_element_ctx));
    tower_nci_ctx.element_count = block_count;

    return FWK_SUCCESS;
}

static int mod_tower_nci_element_init(fwk_id_t element_id, unsigned int unused,
                                   const void *data)
{
    struct tower_nci_config *config;
    unsigned int idx;

    config = (struct tower_nci_config *)data;
    if ((config == NULL) || (config->base == 0)) {
        fwk_unexpected();
        return FWK_E_DATA;
    }

    if (config->base == 0) {
        /* Invalid element. The driver can either program during runtime or
         * init. If the element is invalid then that means there are no regions
         * to configure during runtime
         */
        return FWK_SUCCESS;
    }

    idx = fwk_id_get_element_idx(element_id);
    tower_nci_ctx.element_ctx[idx].config = config;

    return FWK_SUCCESS;
}

static int mod_tower_nci_start(fwk_id_t id)
{
    struct tower_nci_config *config;

    if (fwk_id_get_type(id) == FWK_ID_TYPE_MODULE) {
        return FWK_SUCCESS;
    }

    fwk_assert(fwk_module_is_valid_element_id(id));

    config = tower_nci_ctx.element_ctx[fwk_id_get_element_idx(id)].config;
    return fwk_notification_subscribe(
            mod_clock_notification_id_state_changed, config->clock_id, id);

}

static int mod_tower_nci_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    struct clock_notification_params *params;
    struct mod_tower_nci_element_ctx *element_ctx;
    struct tower_nci_asni_config *psam_mmap;
    uint32_t asni_sam_base;
    uint8_t idx;
    int status;

    fwk_assert(
        fwk_id_is_equal(event->id, mod_clock_notification_id_state_changed));
    fwk_assert(fwk_id_is_type(event->target_id, FWK_ID_TYPE_ELEMENT));

    params = (struct clock_notification_params *)event->params;
    if (params->new_state == MOD_CLOCK_STATE_RUNNING) {
        element_ctx =
           &tower_nci_ctx.element_ctx[fwk_id_get_element_idx(event->target_id)];

        psam_mmap = element_ctx->config->psam_mmap;
        for (idx = 0; idx < element_ctx->config->asni_count;  idx++) {
            asni_sam_base = mod_tower_nci_get_subfeature_address(element_ctx->config->base, 0, TOWER_NCI_NODE_TYPE_ASNI, psam_mmap[idx].id, TOWER_NCI_SUBFEATURE_TYPE_PSAM);
            status = tower_nci_program_psam_regions(asni_sam_base,
                                                    psam_mmap[idx].region,
                                                   psam_mmap[idx].region_count);
            if (status != FWK_SUCCESS) {
                return status;
            }
        }
    }

    return fwk_notification_unsubscribe(
            event->id, event->source_id, event->target_id);
}

static int map_region_in_psam(uintptr_t base,
                              struct tower_nci_asni_config *ansi_map)
{
    struct tower_nci_psam_region *psam_regions;
    uint64_t region;
    uint32_t asni_sam_base;
    uint32_t region_count;
    int status;

    asni_sam_base =
        mod_tower_nci_get_subfeature_address(base, 0,
                TOWER_NCI_NODE_TYPE_ASNI, ansi_map->id, TOWER_NCI_SUBFEATURE_TYPE_PSAM);
    if (asni_sam_base == 0) {
        return FWK_E_DATA;
    }

    psam_regions = ansi_map->region;
    for (region_count = 0; region_count < ansi_map->region_count;
         region_count++) {
        status = tower_nci_find_region_in_psam(
                                asni_sam_base,
                                psam_regions[region_count].base_address,
                                psam_regions[region_count].node_id, &region);
        if (status != FWK_SUCCESS) {
            return status;
        }

        status = tower_nci_psam_disable(asni_sam_base);
        if (status != FWK_SUCCESS) {
            return status;
        }

        status = tower_nci_psam_nhregion_init(asni_sam_base,
                psam_regions[region_count].base_address,
                (psam_regions[region_count].base_address +
                 psam_regions[region_count].size) - 1,
                psam_regions[region_count].node_id, region);
        if (status != FWK_SUCCESS) {
            return status;
        }

        status = tower_nci_psam_enable(asni_sam_base);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return FWK_SUCCESS;
}

static struct mod_tower_nci_memmap_api tower_nci_memmap_api = {
    .map_region_in_psam = map_region_in_psam,
};

static int mod_tower_nci_process_bind_request(
        fwk_id_t requester_id,
        fwk_id_t targer_id,
        fwk_id_t api_id,
        const void **api)
{
    int status;

    status = FWK_SUCCESS;
    if (fwk_id_get_api_idx(api_id) == MOD_TOWER_NCI_API_MAP_PSAM) {
        *api = &tower_nci_memmap_api;
    } else {
        status = FWK_E_DATA;
    }

    return status;
}

const struct fwk_module module_tower_nci = {
    .api_count = MOD_TOWER_NCI_API_COUNT,
    .type = FWK_MODULE_TYPE_DRIVER,
    .event_count = 0,
    .init = mod_tower_nci_init,
    .element_init = mod_tower_nci_element_init,
    .start = mod_tower_nci_start,
    .process_notification = mod_tower_nci_process_notification,
    .process_bind_request = mod_tower_nci_process_bind_request,
};
