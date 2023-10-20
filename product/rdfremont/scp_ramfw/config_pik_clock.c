/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'pik_clock'.
 */

#include "scp_clock.h"
#include "core_manager.h"
#include "scp_pwrctrl.h"
#include "system_pik.h"

#include <mod_pik_clock.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>

#include <stdbool.h>

#define CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(n) \
    [CFGD_MOD_PIK_CLOCK_EIDX_CPU##n] = { \
        .name = "PIK CLK CPU" #n, \
        .data = &((struct mod_pik_clock_dev_config){ \
            .type = MOD_PIK_CLOCK_TYPE_CLUSTER, \
            .is_group_member = true, \
            .control_reg = &SCP_CLUSTER_UTILITY_CORE_MANAGER_PTR(n)->CORECLK_CTRL, \
            .divext_reg = &SCP_CLUSTER_UTILITY_CORE_MANAGER_PTR(n)->CORECLK_DIV1, \
            .modulator_reg = &SCP_CLUSTER_UTILITY_CORE_MANAGER_PTR(n)->CORECLK_MOD1, \
            .rate_table = rate_table_cpu_group, \
            .rate_count = FWK_ARRAY_SIZE(rate_table_cpu_group), \
        }), \
    }

/*
 * Rate lookup tables
 */
static struct mod_pik_clock_rate rate_table_cpu_group[] = {
    {
        .rate = 2600 * FWK_MHZ,
        .source = MOD_PIK_CLOCK_CLUSCLK_SOURCE_PLL0,
        .divider_reg = MOD_PIK_CLOCK_MSCLOCK_DIVIDER_DIV_EXT,
        .divider = 1,
    },
};

static const struct mod_pik_clock_rate rate_table_sys_intclk[] = {
    {
        .rate = 2000 * FWK_MHZ,
        .source = MOD_PIK_CLOCK_INTCLK_SOURCE_INTPLL,
        .divider_reg = MOD_PIK_CLOCK_MSCLOCK_DIVIDER_DIV_EXT,
        .divider = 1,
    },
};

static const struct mod_pik_clock_rate rate_table_scp[] = {
    {
        .rate = 800 * FWK_MHZ,
        .source = MOD_PIK_CLOCK_MSCLOCK_SOURCE_SYSPLLCLK,
        .divider_reg = MOD_PIK_CLOCK_MSCLOCK_DIVIDER_DIV_SYS,
        .divider = CLOCK_RATE_SYSPLLCLK / (800 * FWK_MHZ),
    },
};

static const struct mod_pik_clock_rate rate_table_gicclk[] = {
    {
        .rate = 1000 * FWK_MHZ,
        .source = MOD_PIK_CLOCK_MSCLOCK_SOURCE_SYSPLLCLK,
        .divider_reg = MOD_PIK_CLOCK_MSCLOCK_DIVIDER_DIV_SYS,
        .divider = CLOCK_RATE_SYSPLLCLK / (1000 * FWK_MHZ),
    },
};

static const struct mod_pik_clock_rate rate_table_pclkscp[] = {
    {
        .rate = 400 * FWK_MHZ,
        .source = MOD_PIK_CLOCK_MSCLOCK_SOURCE_SYSPLLCLK,
        .divider_reg = MOD_PIK_CLOCK_MSCLOCK_DIVIDER_DIV_SYS,
        .divider = CLOCK_RATE_SYSPLLCLK / (400 * FWK_MHZ),
    },
};

static const struct mod_pik_clock_rate rate_table_sysperclk[] = {
    {
        .rate = 500 * FWK_MHZ,
        .source = MOD_PIK_CLOCK_MSCLOCK_SOURCE_SYSPLLCLK,
        .divider_reg = MOD_PIK_CLOCK_MSCLOCK_DIVIDER_DIV_SYS,
        .divider = CLOCK_RATE_SYSPLLCLK / (500 * FWK_MHZ),
    },
};

static const struct mod_pik_clock_rate rate_table_uartclk[] = {
    {
        .rate = 250 * FWK_MHZ,
        .source = MOD_PIK_CLOCK_MSCLOCK_SOURCE_SYSPLLCLK,
        .divider_reg = MOD_PIK_CLOCK_MSCLOCK_DIVIDER_DIV_SYS,
        .divider = CLOCK_RATE_SYSPLLCLK / (250 * FWK_MHZ),
    },
};

static const struct fwk_element pik_clock_element_table[] = {
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(0),
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(1),
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(2),
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(3),
#if (PLATFORM_VARIANT == 0 || PLATFORM_VARIANT == 1)
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(4),
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(5),
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(6),
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(7),
#endif
#if (PLATFORM_VARIANT == 0)
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(8),
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(9),
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(10),
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(11),
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(12),
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(13),
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(14),
    CFGD_MOD_PIK_CLOCK_ELEMENT_CPU(15),
#endif
    [CFGD_MOD_PIK_CLOCK_EIDX_CMN] = {
        .name = "PIK CLK CMN",
        .data = &((struct mod_pik_clock_dev_config) {
            .type = MOD_PIK_CLOCK_TYPE_MULTI_SOURCE,
            .is_group_member = false,
            .control_reg = &SYSTEM_PIK_PTR->INTCLK_CTRL,
            .divext_reg = &SYSTEM_PIK_PTR->INTCLK_DIV1,
            .rate_table = rate_table_sys_intclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_sys_intclk),
            .initial_rate = 2000 * FWK_MHZ,
        }),
    },
    [CFGD_MOD_PIK_CLOCK_EIDX_SCP] = {
        .name = "PIK CLK SCP",
        .data = &((struct mod_pik_clock_dev_config) {
            .type = MOD_PIK_CLOCK_TYPE_MULTI_SOURCE,
            .is_group_member = false,
            .control_reg = &SCP_PWRCTRL_PTR->CORECLK_CTRL,
            .divsys_reg = &SCP_PWRCTRL_PTR->CORECLK_DIV1,
            .rate_table = rate_table_scp,
            .rate_count = FWK_ARRAY_SIZE(rate_table_scp),
            .initial_rate = 800 * FWK_MHZ,
        }),
    },
    [CFGD_MOD_PIK_CLOCK_EIDX_GIC] = {
        .name = "PIK CLK GIC",
        .data = &((struct mod_pik_clock_dev_config) {
            .type = MOD_PIK_CLOCK_TYPE_MULTI_SOURCE,
            .is_group_member = false,
            .control_reg = &SYSTEM_PIK_PTR->GICCLK_CTRL,
            .divsys_reg = &SYSTEM_PIK_PTR->GICCLK_DIV1,
            .rate_table = rate_table_gicclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_gicclk),
            .initial_rate = 1000 * FWK_MHZ,
        }),
    },
    [CFGD_MOD_PIK_CLOCK_EIDX_PCLKSCP] = {
        .name = "PIK CLK PCLKSCP",
        .data = &((struct mod_pik_clock_dev_config) {
            .type = MOD_PIK_CLOCK_TYPE_MULTI_SOURCE,
            .is_group_member = false,
            .control_reg = &SYSTEM_PIK_PTR->SCPPIKCLK_CTRL,
            .divsys_reg = &SYSTEM_PIK_PTR->SCPPIKCLK_DIV1,
            .rate_table = rate_table_pclkscp,
            .rate_count = FWK_ARRAY_SIZE(rate_table_pclkscp),
            .initial_rate = 400 * FWK_MHZ,
        }),
    },
    [CFGD_MOD_PIK_CLOCK_EIDX_SYSPERCLK] = {
        .name = "PIK CLK SYSPERCLK",
        .data = &((struct mod_pik_clock_dev_config) {
            .type = MOD_PIK_CLOCK_TYPE_MULTI_SOURCE,
            .is_group_member = false,
            .control_reg = &SYSTEM_PIK_PTR->SYSPERCLK_CTRL,
            .divsys_reg = &SYSTEM_PIK_PTR->SYSPERCLK_DIV1,
            .rate_table = rate_table_sysperclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_sysperclk),
            .initial_rate = 500 * FWK_MHZ,
        }),
    },
    [CFGD_MOD_PIK_CLOCK_EIDX_UARTCLK] = {
        .name = "PIK CLK UARTCLK",
        .data = &((struct mod_pik_clock_dev_config) {
            .type = MOD_PIK_CLOCK_TYPE_MULTI_SOURCE,
            .is_group_member = false,
            .control_reg = &SYSTEM_PIK_PTR->APUARTCLK_CTRL,
            .divsys_reg = &SYSTEM_PIK_PTR->APUARTCLK_DIV1,
            .rate_table = rate_table_uartclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_uartclk),
            .initial_rate = 250 * FWK_MHZ,
        }),
    },
    [CFGD_MOD_PIK_CLOCK_EIDX_COUNT] = { 0 },
};

static const struct fwk_element *pik_clock_get_element_table(fwk_id_t module_id)
{
    return pik_clock_element_table;
}

const struct fwk_module_config config_pik_clock = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(pik_clock_get_element_table),
};
