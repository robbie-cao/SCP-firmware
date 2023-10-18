/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions for SDS module configuration data in SCP firmware.
 */

#ifndef SCP_CFGD_SDS_H
#define SCP_CFGD_SDS_H

#include "platform_core.h"

#include <mod_pcie_setup.h>
#include <mod_sds.h>

#include <stdint.h>

/* SDS structure identifiers. */
enum scp_cfgd_mod_sds_struct_id {
    SDS_AP_CPU_INFO_STRUCT_ID = 1 | (1 << MOD_SDS_ID_VERSION_MAJOR_POS),
    SDS_ROM_VERSION_STRUCT_ID = 2 | (1 << MOD_SDS_ID_VERSION_MAJOR_POS),
    SDS_RAM_VERSION_STRUCT_ID = 3 | (1 << MOD_SDS_ID_VERSION_MAJOR_POS),
    SDS_RESET_SYNDROME_STRUCT_ID = 5 | (1 << MOD_SDS_ID_VERSION_MAJOR_POS),
    SDS_FEATURE_AVAIL_STRUCT_ID = 6 | (1 << MOD_SDS_ID_VERSION_MAJOR_POS),
    SDS_PCIE_MMAP_INFO = 129 | (1 << MOD_SDS_ID_VERSION_MAJOR_POS),
};

/* Memory region identifiers that hold the SDS structures. */
enum scp_cfgd_mod_sds_region_idx {
    SCP_CFGD_MOD_SDS_REGION_IDX_SECURE,
    SCP_CFGD_MOD_SDS_REGION_IDX_COUNT,
};

/* Module 'sds' element indexes (SDS region descriptors) */
enum scp_cfgd_mod_sds_element_idx {
    SCP_CFGD_MOD_SDS_EIDX_CPU_INFO,
    SCP_CFGD_MOD_SDS_EIDX_ROM_VERSION,
    SCP_CFGD_MOD_SDS_EIDX_RAM_VERSION,
    SCP_CFGD_MOD_SDS_EIDX_RESET_SYNDROME,
    SCP_CFGD_MOD_SDS_EIDX_FEATURE_AVAILABILITY,
    SCP_CFGD_MOD_SDS_EIDX_PCIE_MMAP,
    SCP_CFGD_MOD_SDS_EIDX_COUNT
};

/* SDS region descriptor structure sizes. */
#define SCP_CFGD_MOD_SDS_CPU_INFO_SIZE             4
#define SCP_CFGD_MOD_SDS_ROM_VERSION_SIZE          4
#define SCP_CFGD_MOD_SDS_RAM_VERSION_SIZE          4
#define SCP_CFGD_MOD_SDS_RESET_SYNDROME_SIZE       4
#define SCP_CFGD_MOD_SDS_FEATURE_AVAILABILITY_SIZE 4
#define SCP_CFGD_MOD_SDS_PCIE_MMAP_SIZE            \
    (PLATFORM_CHIP_COUNT * NUM_PCIE_INTEG_CTRL * \
     sizeof(struct mod_pcie_setup_ep_sds_info))

/* Flags to indicate the available features */
#define PLATFORM_SDS_FEATURE_FIRMWARE_MASK  0x1

#endif /* SCP_CFGD_SDS_H */
