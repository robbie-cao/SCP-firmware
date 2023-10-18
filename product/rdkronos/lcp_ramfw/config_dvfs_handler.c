/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lcp_mhu3.h>
#include <lcp_mmap.h>

#include <mod_dvfs_handler.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

static struct mod_dvfs_handler_opp dvfs_handler_opps[] = {
    { .level = 85UL, .frequency = 1700 * FWK_MHZ, .voltage = 850000 },
    { .level = 100UL, .frequency = 2000 * FWK_MHZ, .voltage = 900000 },
    { .level = 115UL, .frequency = 2300 * FWK_MHZ, .voltage = 950000 },
    { .level = 130UL, .frequency = 2600 * FWK_MHZ, .voltage = 950000 },
    { .level = 145UL, .frequency = 2900 * FWK_MHZ, .voltage = 1000000 },
    { .level = 160UL, .frequency = 3200 * FWK_MHZ, .voltage = 1050000 },
    { 0 }
};

static const struct fwk_element element_table[] = {
    [0] = {
        .name = "CPU-Domain",
        .data = &((struct mod_dvfs_handler_config) {
            .sustained_idx = 4,
            .dvfs_handler_addr = LCP_DVFS_FRAME_BASE,
            .opps = dvfs_handler_opps,
#ifdef BUILD_HAS_FAST_CHANNELS
            .dvfs_fch_set_level = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_MHU3,
                MHU3_DEVICE_IDX_LCP_AP_FCH_DVFS_SET_LVL),
	    .dvfs_fch_set_limit_min = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_MHU3,
                MHU3_DEVICE_IDX_LCP_AP_FCH_DVFS_SET_LIM_MIN),
	    .dvfs_fch_set_limit_max = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_MHU3,
                MHU3_DEVICE_IDX_LCP_AP_FCH_DVFS_SET_LIM_MAX),
#endif
        }),
    },
    [1] = { 0 },
};

static const struct fwk_element *dvfs_handler_get_element_table(fwk_id_t module_id)
{
    return element_table;
}

const struct fwk_module_config config_dvfs_handler = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(dvfs_handler_get_element_table)
};
