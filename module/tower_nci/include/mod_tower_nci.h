/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef MOD_TOWER_NCI_H
#define MOD_TOWER_NCI_H

#include <fwk_id.h>

#include <stdint.h>

struct tower_nci_psam_region {
    uint32_t node_id;
    uint64_t base_address;
    uint64_t size;
};

struct tower_nci_asni_config {
    uint32_t id;
    struct tower_nci_psam_region *region;
    uint32_t region_count;
};

struct tower_nci_config {
   uint32_t base;
   struct tower_nci_asni_config *psam_mmap;
   uint32_t asni_count;
   fwk_id_t clock_id;
};

/**
 * \brief Tower NCI node type enumerations
 */
enum tower_nci_node_type {
    // Domains
    TOWER_NCI_NODE_TYPE_CFGNI,
    TOWER_NCI_NODE_TYPE_VD,
    TOWER_NCI_NODE_TYPE_PD,
    TOWER_NCI_NODE_TYPE_CD,
    // Components
    TOWER_NCI_NODE_TYPE_ASNI,
    TOWER_NCI_NODE_TYPE_AMNI,
    TOWER_NCI_NODE_TYPE_PMU,
    TOWER_NCI_NODE_TYPE_HSNI,
    TOWER_NCI_NODE_TYPE_HMNI,
    TOWER_NCI_NODE_TYPE_PMNI,
    TOWER_NCI_NODE_TYPE_MAX
};

/**
 * \brief Tower NCI subfeature type enumerations
 */
enum mod_tower_nci_subfeatue_type {
    // Sub-features
    TOWER_NCI_SUBFEATURE_TYPE_APU,
    TOWER_NCI_SUBFEATURE_TYPE_PSAM,
    TOWER_NCI_SUBFEATURE_TYPE_FCU,
    TOWER_NCI_SUBFEATURE_TYPE_IDM,
};

enum mod_tower_nci_api_idx {
    MOD_TOWER_NCI_API_MAP_PSAM,
    MOD_TOWER_NCI_API_COUNT
};

/*!
 * \brief Module interface to manage mappings.
 */
struct mod_tower_nci_memmap_api {
/*!
 *
 * \brief Program a range of memory map regionns in the target tower_nci
 *        instance.
 *
 * \param base Base address of the NCI configuration registers.
 * \param ansi_map list of the regions to be mapped.
 *
 * \return FWK_E_DATA if mapping region is invalid.
 * \return FWK_E_SUCESS if regions are mapped succesfully.
 */
    int (*map_region_in_psam)(uintptr_t base,
            struct tower_nci_asni_config *ansi_map);
};

#endif /* MOD_TOWER_NCI_H */
