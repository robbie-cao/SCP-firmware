/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Module for configuring PCIe rootports and its memory map.
 */

#ifndef MOD_PCIE_SETUP_H
#define MOD_PCIE_SETUP_H

#include <mod_tower_nci.h>

#include <fwk_id.h>
#include <fwk_module_idx.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*!
 * \brief Type of device attached to PCIe integration controller.
 */
enum mod_pcie_setup_dev_type {
    MOD_PCIE_SETUP_DEV_PCIE,
    MOD_PCIE_SETUP_DEV_NON_PCIE
};
/*!
 * \brief Defines base address and size of carveout.
 */
struct mod_pcie_setup_carveout {
    /*! Start of the carveout */
    uint64_t start;
    /*! Size of the carveout */
    uint64_t size;
};
/*!
 * \brief Defines regions for ECAM, MMIOL, MMIOH and Bus.
 */
struct mod_pcie_setup_mmap {
    struct mod_pcie_setup_carveout ecam1;
    struct mod_pcie_setup_carveout mmiol;
    struct mod_pcie_setup_carveout mmioh;
    struct mod_pcie_setup_carveout bus;
    uint64_t base_interrupt_id;
};
/*!
 * \brief Placeholder to put ECAM, MMIOL, MMIOH and Bus number info for each RP.
 */

struct mod_pcie_setup_ep_mmap {
    struct mod_pcie_setup_mmap x1;
    struct mod_pcie_setup_mmap x2_0;
    struct mod_pcie_setup_mmap x2_1;
    struct mod_pcie_setup_mmap x4;
    struct mod_pcie_setup_mmap x8;
};

/*!
 * \brief End point configuration to be passed to the next stage binary via SDS.
 */
struct mod_pcie_setup_ep_sds_info {
    struct mod_pcie_setup_ep_mmap mmap;
    uint64_t segment;
    uint64_t hostbridge_id;
    uint64_t translation;
};

/*!
 * \brief configuration for a single end point and memory map for non io macro.
 */
struct mod_pcie_setup_ep_config {
    /*! Valid flag to indicate if the mapping is valid to program */
    bool valid;
    /*! Allow non-secure access
     *
     * By default only secure accesses are allowed. Set this to true to allow
     * non-secure access as well.
     */
    bool allow_ns_access;
    /*! Memory map capturing region info for the non pcie IO macro. */
    struct mod_pcie_setup_carveout non_pcie_io_macro_region;
};

/*!
 * \brief Module configuration data used for per chip resource allocation. */
struct mod_pcie_setup_resource_info {
    /*! Address space size assigned to each chip. */
    uint64_t chip_address_space;

    /*! Memory map information for each of the root complex in IO macro. */
    struct mod_pcie_setup_mmap mmap;
    uint64_t x1_base_interrupt_id;
    uint64_t x2_0_base_interrupt_id;
    uint64_t x2_1_base_interrupt_id;
    uint64_t x4_base_interrupt_id;
    uint64_t x8_base_interrupt_id;

    /*! Transport channel IDs for cross-chip SCP communication.
     *
     * Each SCP has 3 transport channels for cross-chip inter-scp communication.
     * So, the identifiers corresponding to the cross-chip transport channels,
     * as defined in the transport module should be passed via config data.
     */
    fwk_id_t transport_ids[3];

    /*! Identifier of the SDS structure to place PCIE mmap info. */
    uint32_t sds_struct_id;
    /*! Base address of mapped ECAM address in ATU. */
    uintptr_t mapped_ecam_base;
    /*! Base address of mapped NCI GVP register region address in ATU. */
    uintptr_t mapped_nci_gvp_base;
    /*! Size of mapped NCI GVP register region address in ATU. */
    size_t mapped_nci_gvp_size;
    /*! target amni id. */
    unsigned int x1_amni_id;
    unsigned int x2_0_amni_id;
    unsigned int x2_1_amni_id;
    unsigned int x4_amni_id;
    unsigned int x8_amni_id;
    /*! Id of the target asni_node. */
    unsigned int asni_id;
};

/*!
 * \brief PCIe Integration control registers configuration data which contains
 * mapping of x4_0, x4_1, x8 and x16.
 */
struct mod_pcie_setup_config {
    /*! Type of the device that is attached to the PCIe integration controller.
     *
     *  For PCIe device mmio carveouts size and address will be generated during
     *  runtime.
     *
     *  For Non-PCIe device carveout size and address will be fixed during
     *  compile time.
     */
    enum mod_pcie_setup_dev_type type;
    /*! End point config for x1 */
    struct mod_pcie_setup_ep_config x1;
    /*! End point config for x2_0 */
    struct mod_pcie_setup_ep_config x2_0;
    /*! End point config for x2_1 */
    struct mod_pcie_setup_ep_config x2_1;
    /*! End point config for x4 */
    struct mod_pcie_setup_ep_config x4;
    /*! End point config for x8 */
    struct mod_pcie_setup_ep_config x8;
    /*! ID of the node in the CMN mesh configuration */
    unsigned int cmn_node_id;
    /*! NCI GVP base */
    uint64_t reg_base;
    /*! Identifier of the clock that this module depends on */
    fwk_id_t clock_id;
    /*!
     * Identifier of the SDS structure to place PCIE mmap info.
     */
    uint32_t sds_struct_id;

    /*! Id of the IO Macro block */
    uint64_t hostbridge_id;

    /*! Region for mapping register mapping of TCU & SMMU. */
    struct tower_nci_psam_region *reg_map;
};

/*!
 * \brief PCIe Integ ctrl notification indices.
 */
enum mod_pcie_setup_notification_idx {
    /*! The SDS region has been updated */
    MOD_PCIE_SETUP_NOTIFICATION_IDX_SDS_UPDATED,

    /*! Number of defined notifications */
    MOD_PCIE_SETUP_NOTIFICATION_IDX_COUNT
};

/*!
 * \brief Identifier for the ::MOD_SDS_NOTIFICATION_IDX_INITIALIZED
 *     notification.
 */
static const fwk_id_t mod_pcie_setup_notification_sds_updated =
    FWK_ID_NOTIFICATION_INIT(
        FWK_MODULE_IDX_PCIE_SETUP,
        MOD_PCIE_SETUP_NOTIFICATION_IDX_SDS_UPDATED);

/*!
 * \brief Indices of the interfaces exposed by the module.
 */
enum mod_pcie_setup_api_idx {
    /*! API index for the driver interface of the TRANSPORT module */
    MOD_PCIE_SETUP_API_IDX_TRANSPORT,

    /*! Number of exposed interfaces */
    MOD_PCIE_SETUP_API_COUNT,
};

#endif /* MOD_PCIE_SETUP_H */
