/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Base address definitions for the SCP's expansion memory regions.
 */

#ifndef SCP_EXP_MMAP_H
#define SCP_EXP_MMAP_H

#include "scp_css_mmap.h"

/* PLLs are connected in SCP's expansion3 memory region */
#define SCP_PLL_BASE (SCP_SOC_EXPANSION3_BASE + 0x03000000)

#define SCP_PLL_SYSPLL       (SCP_PLL_BASE + 0x00000000)
#define SCP_PLL_INTERCONNECT (SCP_PLL_BASE + 0x00000020)
#define SCP_PLL_STATUS0      (SCP_PLL_BASE + 0x00000180)
#define SCP_PLL_STATUS1      (SCP_PLL_BASE + 0x00000184)

#define SCP_PLL_CPU0  (SCP_PLL_BASE + 0x00000100)
#define SCP_PLL_CPU1  (SCP_PLL_BASE + 0x00000104)
#define SCP_PLL_CPU2  (SCP_PLL_BASE + 0x00000108)
#define SCP_PLL_CPU3  (SCP_PLL_BASE + 0x0000010C)
#define SCP_PLL_CPU4  (SCP_PLL_BASE + 0x00000110)
#define SCP_PLL_CPU5  (SCP_PLL_BASE + 0x00000114)
#define SCP_PLL_CPU6  (SCP_PLL_BASE + 0x00000118)
#define SCP_PLL_CPU7  (SCP_PLL_BASE + 0x0000011C)
#define SCP_PLL_CPU8  (SCP_PLL_BASE + 0x00000120)
#define SCP_PLL_CPU9  (SCP_PLL_BASE + 0x00000124)
#define SCP_PLL_CPU10 (SCP_PLL_BASE + 0x00000128)
#define SCP_PLL_CPU11 (SCP_PLL_BASE + 0x0000012c)
#define SCP_PLL_CPU12 (SCP_PLL_BASE + 0x00000130)
#define SCP_PLL_CPU13 (SCP_PLL_BASE + 0x00000134)
#define SCP_PLL_CPU14 (SCP_PLL_BASE + 0x00000138)
#define SCP_PLL_CPU15 (SCP_PLL_BASE + 0x0000013C)

/* PLL lock status flag mask */
#define PLL_STATUS_0_SYSPLLLOCK (0x00000002)
#define PLL_STATUS_0_INTPLLLOCK (0x00000008)
#define PLL_STATUS_CPUPLLLOCK(CPU) ((uint32_t)(1 << (CPU % 32)))

#endif /* SCP_EXP_MMAP_H */
