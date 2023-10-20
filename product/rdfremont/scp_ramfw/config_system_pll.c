/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'system_pll'.
 */

#include "scp_clock.h"
#include "scp_pwrctrl.h"
#include "scp_exp_mmap.h"

#include <mod_system_pll.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>

#define CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(n) \
    [CFGD_MOD_SYSTEM_PLL_EIDX_CPU##n] = { \
        .name = "CPU_PLL_" #n, \
        .data = &((struct mod_system_pll_dev_config){ \
            .control_reg = (void *)SCP_PLL_CPU##n, \
            .status_reg = (void *)SCP_PLL_STATUS1, \
            .lock_flag_mask = PLL_STATUS_CPUPLLLOCK(n), \
            .initial_rate = 2600 * FWK_MHZ, \
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE, \
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE, \
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL, \
        }), \
    }

static const struct fwk_element system_pll_element_table[] = {
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(0),
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(1),
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(2),
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(3),
#if (PLATFORM_VARIANT == 0 || PLATFORM_VARIANT == 1)
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(4),
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(5),
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(6),
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(7),
#endif
#if (PLATFORM_VARIANT == 0)
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(8),
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(9),
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(10),
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(11),
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(12),
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(13),
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(14),
    CFGD_MOD_SYSTEM_PLL_ELEMENT_CPU(15),
#endif
    [CFGD_MOD_SYSTEM_PLL_EIDX_SYS] = {
        .name = "SYS_PLL",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SCP_PLL_SYSPLL,
            .status_reg = (void *)SCP_PLL_STATUS0,
            .lock_flag_mask = PLL_STATUS_0_SYSPLLLOCK,
            .initial_rate = 2000 * FWK_MHZ,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_INTERCONNECT] = {
        .name = "INT_PLL",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SCP_PLL_INTERCONNECT,
            .status_reg = (void *)SCP_PLL_STATUS0,
            .lock_flag_mask = PLL_STATUS_0_INTPLLLOCK,
            .initial_rate = 2000 * FWK_MHZ,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_COUNT] = { 0 }, /* Termination description. */
};

static const struct fwk_element *system_pll_get_element_table(
    fwk_id_t module_id)
{
    return system_pll_element_table;
}

const struct fwk_module_config config_system_pll = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(system_pll_get_element_table),
};
