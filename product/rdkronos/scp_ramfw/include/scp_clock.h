/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SCP clock definitions.
 */

#ifndef SCP_CLOCK_H
#define SCP_CLOCK_H

#include "clock_css.h"

#include <fwk_macros.h>

#define CLOCK_RATE_SYSPLLCLK (2000UL * FWK_MHZ)

/*
 * PLL clock indices.
 */
enum clock_pll_idx {
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU0,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU1,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU2,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU3,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU4,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU5,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU6,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU7,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU8,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU9,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU10,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU11,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU12,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU13,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU14,
    CFGD_MOD_SYSTEM_PLL_EIDX_CPU15,
    CFGD_MOD_SYSTEM_PLL_EIDX_SYS,
    CFGD_MOD_SYSTEM_PLL_EIDX_INTERCONNECT,
    CFGD_MOD_SYSTEM_PLL_EIDX_COUNT
};

/*
 * PIK clock indexes.
 */
enum clock_pik_idx {
    CFGD_MOD_PIK_CLOCK_EIDX_CPU0,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU1,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU2,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU3,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU4,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU5,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU6,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU7,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU8,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU9,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU10,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU11,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU12,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU13,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU14,
    CFGD_MOD_PIK_CLOCK_EIDX_CPU15,
    CFGD_MOD_PIK_CLOCK_EIDX_CMN,
    CFGD_MOD_PIK_CLOCK_EIDX_SCP,
    CFGD_MOD_PIK_CLOCK_EIDX_GIC,
    CFGD_MOD_PIK_CLOCK_EIDX_PCLKSCP,
    CFGD_MOD_PIK_CLOCK_EIDX_SYSPERCLK,
    CFGD_MOD_PIK_CLOCK_EIDX_UARTCLK,
    CFGD_MOD_PIK_CLOCK_EIDX_COUNT
};

/*
 * Module 'css_clock' element indexes
 */
enum cfgd_mod_css_clock_element_idx {
    CFGD_MOD_CSS_CLOCK_EIDX_CPU0,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU1,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU2,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU3,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU4,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU5,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU6,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU7,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU8,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU9,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU10,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU11,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU12,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU13,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU14,
    CFGD_MOD_CSS_CLOCK_EIDX_CPU15,
    CFGD_MOD_CSS_CLOCK_EIDX_COUNT
};

/*
 * Module 'clock' element indexes
 */
enum cfgd_mod_clock_element_idx {
    CFGD_MOD_CLOCK_EIDX_CPU0,
    CFGD_MOD_CLOCK_EIDX_CPU1,
    CFGD_MOD_CLOCK_EIDX_CPU2,
    CFGD_MOD_CLOCK_EIDX_CPU3,
    CFGD_MOD_CLOCK_EIDX_CPU4,
    CFGD_MOD_CLOCK_EIDX_CPU5,
    CFGD_MOD_CLOCK_EIDX_CPU6,
    CFGD_MOD_CLOCK_EIDX_CPU7,
    CFGD_MOD_CLOCK_EIDX_CPU8,
    CFGD_MOD_CLOCK_EIDX_CPU9,
    CFGD_MOD_CLOCK_EIDX_CPU10,
    CFGD_MOD_CLOCK_EIDX_CPU11,
    CFGD_MOD_CLOCK_EIDX_CPU12,
    CFGD_MOD_CLOCK_EIDX_CPU13,
    CFGD_MOD_CLOCK_EIDX_CPU14,
    CFGD_MOD_CLOCK_EIDX_CPU15,
    CFGD_MOD_CLOCK_EIDX_CMN,
    CFGD_MOD_CLOCK_EIDX_COUNT
};

#endif /* SCP_CLOCK_H */
