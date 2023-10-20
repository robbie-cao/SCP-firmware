/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FMW_CMSIS_SCP_H
#define FMW_CMSIS_SCP_H

#include <stdint.h>

#define __CHECK_DEVICE_DEFINES
#define __CM7_REV              0x0000U
#define __FPU_PRESENT          0U
#define __MPU_PRESENT          1U
#define __ICACHE_PRESENT       0U
#define __DCACHE_PRESENT       0U
#define __DTCM_PRESENT         0U
#define __NVIC_PRIO_BITS       3U
#define __Vendor_SysTickConfig 0U
#define __VTOR_PRESENT         1U

extern uint32_t SystemCoreClock; /*!< System Clock Frequency (Core Clock)*/

typedef enum IRQn {
    Reset_IRQn = -15,
    NonMaskableInt_IRQn = -14,
    HardFault_IRQn = -13,
    MemoryManagement_IRQn = -12,
    BusFault_IRQn = -11,
    UsageFault_IRQn = -10,
    SVCall_IRQn = -5,
    DebugMonitor_IRQn = -4,
    PendSV_IRQn = -2,
    SysTick_IRQn = -1,

    REFCLK_GTIMER_IRQ = 32, /* SCP REFCLK Physical Timer */
    PPU_CORES0_IRQ =
        50, /* Consolidated PPU Interrupt for cores 1-32, 129-160 */
    PPU_CORES1_IRQ =
        51, /* Consolidated PPU Interrupt for cores 33-64, 161-192 */
    PPU_CORES2_IRQ =
        52, /* Consolidated PPU Interrupt for cores 65-96, 193-224 */
    PPU_CORES3_IRQ =
        53, /* Consolidated PPU Interrupt for cores 97-128, 225-256 */
    PPU_CLUSTERS0_IRQ = 59, /* Consolidate PPU Interrupt for clusters 1-32, 129-160 */
    MHU3_AP2SCP_IRQ_S = 83, /* MHUv3 secure IRQ between SCP and AP */
    MHU3_RSS2SCP_IRQ_S = 86, /* MHUv3 secure IRQ between SCP and RSS */

    IRQn_MAX = INT16_MAX,
} IRQn_Type;

#include <core_cm7.h>

#endif /* FMW_CMSIS_SCP_H */
