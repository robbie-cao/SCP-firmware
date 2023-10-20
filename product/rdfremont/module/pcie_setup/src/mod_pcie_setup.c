/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     RD-Fremont PCIe Setup.
 */

#include <mod_atu.h>
#include <mod_clock.h>
#include <mod_cmn_cyprus.h>
#include <mod_pcie_enumeration.h>
#include <mod_pcie_setup.h>
#include <mod_sds.h>
#include <mod_system_info.h>
#include <mod_tower_nci.h>
#include <mod_transport.h>

#include <fwk_assert.h>
#include <fwk_event.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_mm.h>
#include <fwk_notification.h>
#include <fwk_status.h>

#include <fmw_cmsis.h>

#include <stdbool.h>
#include <stdint.h>

#define MOD_NAME "[PCIE_SETUP]"

struct mod_pcie_setup_context {
    /* Pointer to the element configuration pointers (table) */
    struct mod_pcie_setup_config **config;

    /* APIs required to do PCIe bus walk and resource discovery */
    struct mod_pcie_enumeration_api *pcie_enumeration_api;

    /* Pointer to the module configuration info */
    struct mod_pcie_setup_resource_info resource_info;

    /* APIs to program PCIe memory map configuration info in SDS. */
    struct mod_pcie_setup_ep_sds_info *sds_info;

    /* Pointer to system info APIs */
    struct mod_system_info_get_info_api *system_info_api;

    /* Pointer to system info configuration. */
    const struct mod_system_info *system_info;

    /* APIs to program PCIe memory map configuration info in SDS. */
    struct mod_sds_api *sds_api;

    /* Counter for SDS data received from the remote chip. */
    uint32_t sds_update_count;

    /* API to program PCIe memory region in GVP NCI. */
    struct mod_tower_nci_memmap_api *nci_memmap_api;

    /* APIs to configure CMN moudle. */
    struct mod_cmn_cyprus_rnsam_memmap_api *memmap_rnsam_api;

    /* APIs to memory map regions during runtime. */
    struct mod_atu_api *atu_api;
};

struct mod_pcie_setup_context pcie_setup_context;

static int mod_pcie_setup_init(fwk_id_t module_id, unsigned int block_count,
                               const void *data)
{
    if (block_count == 0) {
        /* There must be at least one pcie integ config data */
        fwk_unexpected();
        return FWK_E_PARAM;
    }

    if (data == NULL) {
        fwk_unexpected();
        return FWK_E_DATA;
    }

    memcpy(&pcie_setup_context.resource_info, data,
           sizeof(struct mod_pcie_setup_resource_info));
    pcie_setup_context.config = fwk_mm_calloc(block_count,
                                       sizeof(struct mod_pcie_setup_config *));
    if (pcie_setup_context.config == NULL) {
        return FWK_E_NOMEM;
    }

    pcie_setup_context.sds_info = fwk_mm_calloc(block_count,
                                     sizeof(struct mod_pcie_setup_ep_sds_info));
    if (pcie_setup_context.sds_info == NULL) {
        return FWK_E_NOMEM;
    }

    return FWK_SUCCESS;
}

static int mod_pcie_setup_element_init(fwk_id_t element_id, unsigned int unused,
                                       const void *data)
{
    struct mod_pcie_setup_config *config;

    config = (struct mod_pcie_setup_config *)data;
    if ((config == NULL) || (config->reg_base == 0)) {
        fwk_unexpected();
        return FWK_E_DATA;
    }

    pcie_setup_context.config[fwk_id_get_element_idx(element_id)] = config;

    return FWK_SUCCESS;
}

static int mod_pcie_setup_start(fwk_id_t id)
{
    struct mod_pcie_setup_config *config;
    int status;

    if (fwk_id_get_type(id) == FWK_ID_TYPE_MODULE) {
        return FWK_SUCCESS;
    }

    fwk_assert(fwk_module_is_valid_element_id(id));

    status = pcie_setup_context.system_info_api->get_system_info(
            &pcie_setup_context.system_info);
    if (status != FWK_SUCCESS) {
        return status;
    }

    config =
        pcie_setup_context.config[fwk_id_get_element_idx(id)];
    status = fwk_notification_subscribe(
            mod_sds_notification_id_initialized, fwk_module_id_sds, id);
    if (status != FWK_SUCCESS) {
        return status;
    }

    return fwk_notification_subscribe(mod_clock_notification_id_state_changed,
                                      config->clock_id, id);
}

static int mod_pcie_setup_bind(fwk_id_t id, unsigned int round)
{
    int status;

    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_CMN_CYPRUS),
        FWK_ID_API(FWK_MODULE_IDX_CMN_CYPRUS,
                   MOD_CMN_CYPRUS_API_IDX_MAP_IO_REGION),
        &pcie_setup_context.memmap_rnsam_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SYSTEM_INFO),
        FWK_ID_API(FWK_MODULE_IDX_SYSTEM_INFO, MOD_SYSTEM_INFO_GET_API_IDX),
        &pcie_setup_context.system_info_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_TOWER_NCI),
        FWK_ID_API(FWK_MODULE_IDX_TOWER_NCI, MOD_TOWER_NCI_API_MAP_PSAM),
        &pcie_setup_context.nci_memmap_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (pcie_setup_context.sds_api == NULL) {
        status = fwk_module_bind(
            FWK_ID_MODULE(FWK_MODULE_IDX_SDS),
            FWK_ID_API(FWK_MODULE_IDX_SDS, 0),
            &pcie_setup_context.sds_api);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    if (pcie_setup_context.atu_api == NULL) {
        status = fwk_module_bind(
            FWK_ID_ELEMENT(FWK_MODULE_IDX_ATU, 0),
            FWK_ID_API(FWK_MODULE_IDX_ATU, MOD_ATU_API_IDX_ATU),
            &pcie_setup_context.atu_api);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return fwk_module_bind(FWK_ID_MODULE(FWK_MODULE_IDX_PCIE_ENUMERATION),
            FWK_ID_API(FWK_MODULE_IDX_PCIE_ENUMERATION, 0),
            &pcie_setup_context.pcie_enumeration_api);
}

static int pcie_setup_update_sds(
    struct mod_pcie_setup_ep_sds_info *sds_info,
    const struct mod_pcie_setup_config *config)
{
    int status;
    static uint32_t offset = 0;

    status = pcie_setup_context.sds_api->struct_write(
        config->sds_struct_id,
        offset,
        sds_info,
        sizeof(struct mod_pcie_setup_ep_sds_info));
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("SDS update failed");
    } else {
        offset += sizeof(struct mod_pcie_setup_ep_sds_info);
    }
    return status;
}

static void allocate_ecam_address(
    struct mod_pcie_setup_mmap *mmap,
    struct pcie_mmap_size *mmap_size)
{
    struct mod_pcie_setup_resource_info *res_info;

    res_info = &pcie_setup_context.resource_info;

    if (mmap_size->ecam != 0) {
        if (mmap_size->ecam > res_info->mmap.ecam1.size) {
            FWK_LOG_ERR(MOD_NAME "No memory left to allocate for ECAM");
            fwk_unexpected();
        }
        mmap->ecam1.start = res_info->mmap.ecam1.start;
        mmap->ecam1.size = mmap_size->ecam;
        res_info->mmap.ecam1.start += mmap_size->ecam;
        res_info->mmap.ecam1.size -= mmap_size->ecam;
    } else {
        /*
         * Reset the values to zero to prevent caller from using garbage values.
         */
        mmap->ecam1.start = 0;
        mmap->ecam1.size = 0;
    }
}

static void allocate_mmiol_address(
    struct mod_pcie_setup_mmap *mmap,
    struct pcie_mmap_size *mmap_size)
{
    struct mod_pcie_setup_resource_info *res_info;

    res_info = &pcie_setup_context.resource_info;

    if (mmap_size->mmiol != 0) {
        if (mmap_size->mmiol > res_info->mmap.mmiol.size) {
            FWK_LOG_ERR(MOD_NAME " No memory left to allocate for MMIOL");
            fwk_unexpected();
        }
        mmap->mmiol.start = res_info->mmap.mmiol.start;
        mmap->mmiol.size = mmap_size->mmiol;
        res_info->mmap.mmiol.start += mmap_size->mmiol;
        res_info->mmap.mmiol.size -= mmap_size->mmiol;
    } else {
        /*
         * Reset the values to zero to prevent caller from using garbage values.
         */
        mmap->mmiol.start = 0;
        mmap->mmiol.size = 0;
    }
}

static void allocate_mmioh_address(
    struct mod_pcie_setup_mmap *mmap,
    struct pcie_mmap_size *mmap_size)
{
    struct mod_pcie_setup_resource_info *res_info;

    res_info = &pcie_setup_context.resource_info;

    if (mmap_size->mmioh != 0) {
        if (mmap_size->mmioh > res_info->mmap.mmioh.size) {
            FWK_LOG_ERR(MOD_NAME " No memory left to allocate for MMIOH");
            fwk_unexpected();
        }
        mmap->mmioh.start = res_info->mmap.mmioh.start;
        mmap->mmioh.size = mmap_size->mmioh;
        res_info->mmap.mmioh.start += mmap_size->mmioh;
        res_info->mmap.mmioh.size -= mmap_size->mmioh;
    } else {
        /*
         * Reset the values to zero to prevent caller from using garbage values.
         */
        mmap->mmioh.start = 0;
        mmap->mmioh.size = 0;
    }
}

static void allocate_bus(
    struct mod_pcie_setup_mmap *mmap,
    struct pcie_mmap_size *mmap_size)
{
    struct mod_pcie_setup_resource_info *res_info;

    res_info = &pcie_setup_context.resource_info;

    if (mmap_size->bus != 0) {
        if (mmap_size->bus > res_info->mmap.bus.size) {
            FWK_LOG_ERR(MOD_NAME " No bus left to allocate");
            fwk_unexpected();
        }
        mmap->bus.start = res_info->mmap.bus.start;
        mmap->bus.size = mmap_size->bus;
        res_info->mmap.bus.start += mmap_size->bus;
        res_info->mmap.bus.size -= mmap_size->bus;
    } else {
        /*
         * Reset the values to zero to prevent caller from using garbage values.
         */
        mmap->bus.start = 0;
        mmap->bus.size = 0;
    }
}

/*
 * This takes in the size required for each of the region and then allocates
 * regions for them from platform provided configuration.
 */
static inline void get_address_range(
    struct mod_pcie_setup_mmap *mmap,
    struct pcie_mmap_size *mmap_size)
{
    allocate_ecam_address(mmap, mmap_size);
    allocate_mmiol_address(mmap, mmap_size);
    allocate_mmioh_address(mmap, mmap_size);
    allocate_bus(mmap, mmap_size);
}

static int map_region_in_nci(
        uintptr_t base, uint32_t target_id, uint64_t address, uint64_t size)
{
    struct tower_nci_psam_region psam_regions = {
        .node_id = target_id,
        .base_address = address,
        .size = size,
    };
    struct tower_nci_asni_config ansi_map = {
        .id = pcie_setup_context.resource_info.asni_id,
        .region = &psam_regions,
        .region_count = 1,
    };

    return pcie_setup_context.nci_memmap_api->map_region_in_psam(
            base, &ansi_map);
}

static int map_tcu_smmu_registers(struct mod_pcie_setup_config *config)
{
    struct mod_pcie_setup_resource_info *res_info;
    struct tower_nci_psam_region *reg_map;
    uintptr_t reg_base;
    int status;

    res_info = &pcie_setup_context.resource_info;
    reg_map = config->reg_map;
    reg_base = res_info->mapped_nci_gvp_base;
    while (reg_map->base_address != 0) {
        status = map_region_in_nci(reg_base, reg_map->node_id,
                reg_map->base_address,
                reg_map->size);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR("Register mapping in NCI failed");
            return status;
        }
        reg_map++;
    }

    return FWK_SUCCESS;
}

static int discover_and_configure_pcie_device(
        unsigned int amni_id,
        struct mod_pcie_setup_ep_config *ep_config,
        struct mod_pcie_setup_mmap *mmap)
{
    struct pcie_mmap_size mmap_size = { 0 };
    struct mod_pcie_enumeration_api *pcie_enumeration_api;
    struct mod_pcie_setup_resource_info *res_info;
    uintptr_t reg_base;
    int status;

    res_info = &pcie_setup_context.resource_info;
    reg_base = res_info->mapped_nci_gvp_base;

    status = map_region_in_nci(reg_base, amni_id,
                               res_info->mmap.ecam1.start,
                               res_info->mmap.ecam1.size);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("NCI mapping failed");
        return status;
    }

    pcie_enumeration_api = pcie_setup_context.pcie_enumeration_api;
    pcie_enumeration_api->calculate_resource(
            res_info->mapped_ecam_base,
            &mmap_size, res_info->mmap.bus.start, NULL);
    get_address_range(mmap, &mmap_size);

    if (mmap->ecam1.size != 0) {
        status = map_region_in_nci(reg_base, amni_id,
                mmap->ecam1.start,
                mmap->ecam1.size);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR("ECAM Mapping in NCI failed");
                return status;
        }
    }

    if (mmap->mmioh.size != 0) {
        status = map_region_in_nci(reg_base, amni_id,
                mmap->mmioh.start,
                mmap->mmioh.size);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR("MMIOH Mapping in NCI failed");
                return status;
        }
    }

    if (mmap->mmiol.size != 0) {
        status = map_region_in_nci(reg_base, amni_id,
                mmap->mmiol.start,
                mmap->mmiol.size);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR("MMIOL Mapping in NCI failed");
                return status;
        }
    }

    return FWK_SUCCESS;
}

static int pcie_setup_configure_pcie_controller(
        unsigned int index,
        struct mod_pcie_setup_config *config)
{
    struct atu_region_map atu_map;
    struct mod_atu_api *atu_api;
    struct mod_cmn_cyprus_rnsam_memmap_api *memmap_rnsam_api;
    struct mod_pcie_setup_ep_mmap *ep_mmap;
    struct mod_pcie_setup_resource_info *res_info;
    uint64_t ecam_start;
    uint64_t mmiol_start;
    uint64_t mmioh_start;
    uint32_t chip_id;
    size_t ecam_size;
    size_t mmiol_size;
    size_t mmioh_size;
    int status;
    uint8_t gvp_index;
    uint8_t cmn_index;

    chip_id = pcie_setup_context.system_info->chip_id;
    res_info = &pcie_setup_context.resource_info;
    ep_mmap = &pcie_setup_context.sds_info[index].mmap;
    pcie_setup_context.sds_info[index].hostbridge_id = config->hostbridge_id;
    pcie_setup_context.sds_info[index].segment = chip_id;
    pcie_setup_context.sds_info[index].translation =
        chip_id * res_info->chip_address_space;

    ecam_start = res_info->mmap.ecam1.start;
    mmiol_start = res_info->mmap.mmiol.start;
    mmioh_start = res_info->mmap.mmioh.start;
    ecam_size = 0;
    mmiol_size = 0;
    mmioh_size = 0;

    atu_api = pcie_setup_context.atu_api;
    atu_map.region_owner_id = FWK_ID_MODULE(FWK_MODULE_IDX_PCIE_SETUP);
    atu_map.attributes = ATU_ENCODE_ATTRIBUTES_ROOT_PAS;
    atu_map.log_addr_base = res_info->mapped_nci_gvp_base;
    atu_map.region_size = res_info->mapped_nci_gvp_size;
    atu_map.phy_addr_base = config->reg_base;
    status = atu_api->add_region(&atu_map,
                                 FWK_ID_ELEMENT(FWK_MODULE_IDX_ATU, 0),
                                 &gvp_index);
    if (status != FWK_SUCCESS) {
        FWK_LOG_INFO(MOD_NAME"NCI GVP register map in ATU failed");
        return status;
    }

    atu_map.log_addr_base = res_info->mapped_ecam_base;
    atu_map.region_size = res_info->mmap.ecam1.size;
    atu_map.phy_addr_base = res_info->mmap.ecam1.start;
    atu_map.attributes = ATU_ENCODE_ATTRIBUTES_NON_SECURE_PAS;
    status = atu_api->add_region(&atu_map,
                                 FWK_ID_ELEMENT(FWK_MODULE_IDX_ATU, 0),
                                 &cmn_index);
    if (status != FWK_SUCCESS) {
        FWK_LOG_INFO(MOD_NAME"CMN map in ATU failed");
        return status;
    }

    memmap_rnsam_api = pcie_setup_context.memmap_rnsam_api;
    status = memmap_rnsam_api->map_io_region(
            res_info->mmap.ecam1.start,
            res_info->mmap.ecam1.size,
            config->cmn_node_id);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = map_tcu_smmu_registers(config);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (config->x8.valid) {
        status = discover_and_configure_pcie_device(
                res_info->x8_amni_id,
                &config->x8,
                &ep_mmap->x8);
        if (status != FWK_SUCCESS) {
                return status;
        }

        ep_mmap->x8.base_interrupt_id = res_info->x8_base_interrupt_id;
        ecam_size += ep_mmap->x8.ecam1.size;
        mmiol_size += ep_mmap->x8.mmiol.size;
        mmioh_size += ep_mmap->x8.mmioh.size;
    }

    if (config->x4.valid) {
        status = discover_and_configure_pcie_device(
                res_info->x4_amni_id,
                &config->x4,
                &ep_mmap->x4);
        if (status != FWK_SUCCESS) {
                return status;
        }

        ep_mmap->x4.base_interrupt_id = res_info->x4_base_interrupt_id;
        ecam_size += ep_mmap->x4.ecam1.size;
        mmiol_size += ep_mmap->x4.mmiol.size;
        mmioh_size += ep_mmap->x4.mmioh.size;
    }

    if (config->x2_1.valid) {
        status = discover_and_configure_pcie_device(
                res_info->x2_1_amni_id,
                &config->x2_1,
                &ep_mmap->x2_1);
        if (status != FWK_SUCCESS) {
                return status;
        }

        ep_mmap->x2_1.base_interrupt_id = res_info->x2_1_base_interrupt_id;
        ecam_size += ep_mmap->x2_1.ecam1.size;
        mmiol_size += ep_mmap->x2_1.mmiol.size;
        mmioh_size += ep_mmap->x2_1.mmioh.size;
    }

    if (config->x2_0.valid) {
        status = discover_and_configure_pcie_device(
                res_info->x2_0_amni_id,
                &config->x2_0,
                &ep_mmap->x2_0);
        if (status != FWK_SUCCESS) {
                return status;
        }

        ep_mmap->x2_0.base_interrupt_id = res_info->x2_0_base_interrupt_id;
        ecam_size += ep_mmap->x2_0.ecam1.size;
        mmiol_size += ep_mmap->x2_0.mmiol.size;
        mmioh_size += ep_mmap->x2_0.mmioh.size;
    }

    if (config->x1.valid) {
        status = discover_and_configure_pcie_device(
                res_info->x1_amni_id,
                &config->x1,
                &ep_mmap->x1);
        if (status != FWK_SUCCESS) {
                return status;
        }

        ep_mmap->x1.base_interrupt_id = res_info->x1_base_interrupt_id;
        ecam_size += ep_mmap->x1.ecam1.size;
        mmiol_size += ep_mmap->x1.mmiol.size;
        mmioh_size += ep_mmap->x1.mmioh.size;
    }

    status = atu_api->remove_region(gvp_index,
                                 FWK_ID_ELEMENT(FWK_MODULE_IDX_ATU, 0),
                                 atu_map.region_owner_id);
    if (status != FWK_SUCCESS) {
        FWK_LOG_INFO(MOD_NAME"NCI GVP register unmap in ATU failed");
        return status;
    }

    status = atu_api->remove_region(cmn_index,
                                 FWK_ID_ELEMENT(FWK_MODULE_IDX_ATU, 0),
                                 atu_map.region_owner_id);
    if (status != FWK_SUCCESS) {
        FWK_LOG_INFO(MOD_NAME"CMN unmap in ATU failed");
        return status;
    }

    if (ecam_size != 0) {
        status = memmap_rnsam_api->map_io_region(
                ecam_start,
                ecam_size,
                config->cmn_node_id);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    if (mmiol_size != 0) {
        status = memmap_rnsam_api->map_io_region(
                mmiol_start,
                mmiol_size,
                config->cmn_node_id);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    if (mmioh_size != 0) {
        status = memmap_rnsam_api->map_io_region(
                mmioh_start,
                mmioh_size,
                config->cmn_node_id);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return FWK_SUCCESS;
}

static int configure_pcie_ecam_mmio_space(unsigned int index,
                                          struct mod_pcie_setup_config *config)
{
    int status = FWK_SUCCESS;

    FWK_LOG_INFO(MOD_NAME "Configuring PCIe: %d", index);
    if (config->type == MOD_PCIE_SETUP_DEV_PCIE) {
        status = pcie_setup_configure_pcie_controller(index, config);
    }

    return status;
}

static int mod_pcie_setup_process_notification(
        const struct fwk_event *event,
        struct fwk_event *resp_event)
{
    struct clock_notification_params *params;
    struct mod_pcie_setup_config *config;
    uint8_t index;
    int status;

    fwk_assert(fwk_id_is_type(event->target_id, FWK_ID_TYPE_ELEMENT));
    index = fwk_id_get_element_idx(event->target_id);
    config = pcie_setup_context.config[index];

    if (fwk_id_is_equal(event->id, mod_sds_notification_id_initialized)) {
        if (config->type == MOD_PCIE_SETUP_DEV_PCIE) {
            if (pcie_setup_context.system_info->chip_id == 0) {
                status = pcie_setup_update_sds(
                        &pcie_setup_context.sds_info[index], config);
                if (status != FWK_SUCCESS) {
                    return status;
                }
            }
        }
    }

    if (fwk_id_is_equal(event->id, mod_clock_notification_id_state_changed)) {
        params = (struct clock_notification_params *)event->params;
        if (params->new_state != MOD_CLOCK_STATE_RUNNING) {
            return FWK_SUCCESS;
        }

        /* For now, enable pcie configuration only on Chip 0 */
        if (pcie_setup_context.system_info->chip_id == 0) {
            configure_pcie_ecam_mmio_space(
                fwk_id_get_element_idx(event->target_id), config);
        }
    }

    return fwk_notification_unsubscribe(event->id, event->source_id,
            event->target_id);
}

const struct fwk_module module_pcie_setup = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .init = mod_pcie_setup_init,
    .element_init = mod_pcie_setup_element_init,
    .bind = mod_pcie_setup_bind,
    .start = mod_pcie_setup_start,
    .notification_count =
        (unsigned int)MOD_PCIE_SETUP_NOTIFICATION_IDX_COUNT,
    .process_notification = mod_pcie_setup_process_notification,
};
