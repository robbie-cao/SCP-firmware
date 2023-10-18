/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Base address definitions for the SCP's sub-system and access extending
 *     into the rest of the CSS.
 */

#ifndef SCP_CSS_MMAP_H
#define SCP_CSS_MMAP_H

/* Base address and size of SCP's ITCM */
#define SCP_ITC_RAM_BASE (0x00000000)
#define SCP_ITC_RAM_SIZE (256 * 1024)

/* Base address and size of SCP's DTCM */
#define SCP_DTC_RAM_BASE (0x20000000)
#define SCP_DTC_RAM_SIZE (256 * 1024)

/* SCP sub-system peripherals */
#define SCP_REFCLK_CNTCONTROL_BASE (0x2A430000)
#define SCP_SID_BASE (0x2A4A0000)
#define SCP_REFCLK_CNTCTL_BASE (0x44000000)
#define SCP_REFCLK_CNTBASE0_BASE (0x44001000)
#define SCP_UART_BASE (0x44002000)
#define SCP_SCP2AP_MHUV3_SEND_S_BASE (0x45020000)
#define SCP_AP2SCP_MHUV3_RCV_S_BASE (0x45030000)
#define SCP_SCP2RSS_MHUV3_SEND_S_BASE (0x46000000)
#define SCP_RSS2SCP_MHUV3_RCV_S_BASE (0x46010000)
#define SCP_POWER_CONTROL_BASE (0x50000000)
#define SCP_SYSTEM_PIK_BASE (0x50040000)
#define SCP_PPU_SYS0_BASE (0x50041000)

/* Base address of SCP expansion memory regions */
#define SCP_SOC_EXPANSION3_BASE (0x40000000)  /* 64MB size */

/* SCP addresses mapped via ATU into address translation windows */
#define SCP_ADDRESS_TRANSLATION_WINDOW0_BASE (0x60000000)
#define SCP_ADDRESS_TRANSLATION_WINDOW1_BASE (0xA0000000)

/* Offsets within SCP's Address Translation Window0 */
#define SCP_ATW0_LCP_AND_CLUSTER_UTILITY_OFFSET (0x0)
#define SCP_ATW0_AP_PERIPHERAL_SRAM_OFFSET (0x10000000)
#define SCP_ATW0_SYSTEM_CONTROL_SRAM_OFFSET (0x18000000)

/* Offsets within SCP's Address Translation Window1 */
#define SCP_ATW1_CMN_OFFSET (0x0)

/*
 * LCP subsystem and Cluster Utility memory region that is addressable in the AP
 * memory map between 0x2_0000_0000 - 0x2_0FFF_FFFF is mapped in the SCP address
 * translation window 0 from 0x6000_0000 to 0x6FFF_FFFF via ATU configuration.
 */

/*
 * Size of SCP's view of per-cluster LCP subsystem and utlility memory region.
 */
#define SCP_LCP_AND_CLUSTER_UTILITY_SIZE (0x200000)

/*
 * Offsets of various blocks within LCP subsystem and cluster utility that is
 * mapped into SCP's address translation window 0. These offsets are applicable
 * to each cluster in the system.
 */
#define SCP_LCP_CONTROL_OFFSET (0x21000)
#define SCP_LCP_EXTERNAL_CONTROL_OFFSET (0x70000)
#define SCP_CLUSTER_UTILITY_CORE_MANAGER_OFFSET (0x80000)
#define SCP_CLUSTER_UTILITY_CLUSTER_PPU_OFFSET (0x130000)
#define SCP_CLUSTER_UTILITY_CORE_PPU_OFFSET (0x180000)

/* Core Manager base address for a cluster 'n' */
#define SCP_CLUSTER_UTILITY_CORE_MANAGER_BASE(n) \
    (SCP_ADDRESS_TRANSLATION_WINDOW0_BASE + \
        SCP_ATW0_LCP_AND_CLUSTER_UTILITY_OFFSET + \
        (n * SCP_LCP_AND_CLUSTER_UTILITY_SIZE) + \
        SCP_CLUSTER_UTILITY_CORE_MANAGER_OFFSET)

/* Cluster PPU base address */
#define SCP_CLUSTER_UTILITY_CLUSTER_PPU_BASE(n) \
    (SCP_ADDRESS_TRANSLATION_WINDOW0_BASE + \
        SCP_ATW0_LCP_AND_CLUSTER_UTILITY_OFFSET + \
        (n * SCP_LCP_AND_CLUSTER_UTILITY_SIZE) + \
	SCP_CLUSTER_UTILITY_CLUSTER_PPU_OFFSET)

/* Application core PPU base address */
#define SCP_CLUSTER_UTILITY_CORE_PPU_BASE(n) \
    (SCP_ADDRESS_TRANSLATION_WINDOW0_BASE + \
        SCP_ATW0_LCP_AND_CLUSTER_UTILITY_OFFSET + \
        (n * SCP_LCP_AND_CLUSTER_UTILITY_SIZE) + \
	SCP_CLUSTER_UTILITY_CORE_PPU_OFFSET)

/* Base address of SCP's view of LCP's External Control register block */
#define SCP_LCP_EXTERNAL_CONTROL(n) \
    (SCP_ADDRESS_TRANSLATION_WINDOW0_BASE + \
        SCP_ATW0_LCP_AND_CLUSTER_UTILITY_OFFSET + \
        (n * SCP_LCP_AND_CLUSTER_UTILITY_SIZE) + \
	SCP_LCP_EXTERNAL_CONTROL_OFFSET)
		
/* Base address of SCP's view of LCP's Control register block */
#define SCP_LCP_CONTROL(n) \
    (SCP_ADDRESS_TRANSLATION_WINDOW0_BASE + \
        SCP_ATW0_LCP_AND_CLUSTER_UTILITY_OFFSET + \
        (n * SCP_LCP_AND_CLUSTER_UTILITY_SIZE) + \
        SCP_LCP_CONTROL_OFFSET)

/* Offsets in the LCP control register block */
#define SCP_LCP_CONTROL_CPU_WAIT_OFFSET     0x120

/*
 * System Control SRAM (shared between RSS, MCP, SCP and AP) is mapped by ATU in
 * the SCP address translation window 0 at the address 0x7800_0000.
 */
#define SCP_SYSTEM_CONTROL_SRAM_BASE (SCP_ADDRESS_TRANSLATION_WINDOW0_BASE + \
    SCP_ATW0_SYSTEM_CONTROL_SRAM_OFFSET)

/* CMN config space is mapped in the SCP address translation window 1 */
#define SCP_CMN_BASE (SCP_ADDRESS_TRANSLATION_WINDOW1_BASE + \
    SCP_ATW1_CMN_OFFSET)

/* Safety Island CPU layout */
#define SCP_SI_CL0_ID 0
#define SCP_SI_CL0_CORE_NUM 1
#define SCP_SI_CL0_CORE_OFS 1
#define SCP_SI_CL1_ID 1
#define SCP_SI_CL1_CORE_NUM 2
#define SCP_SI_CL1_CORE_OFS 3
#define SCP_SI_CL2_ID 2
#define SCP_SI_CL2_CORE_NUM 4
#define SCP_SI_CL2_CORE_OFS 6

/* Safety Island CPU PPU base addresses */
#define SCP_PPU_SI_CLUS0 (0x56010000)
#define SCP_PPU_SI_CLUS0CORE0 (0x56040000)
#define SCP_PPU_SI_CLUS1 (0x56410000)
#define SCP_PPU_SI_CLUS1CORE0 (0x56440000)
#define SCP_PPU_SI_CLUS1CORE1 (0x56540000)
#define SCP_PPU_SI_CLUS2 (0x56810000)
#define SCP_PPU_SI_CLUS2CORE0 (0x56840000)
#define SCP_PPU_SI_CLUS2CORE1 (0x56940000)
#define SCP_PPU_SI_CLUS2CORE2 (0x56A40000)
#define SCP_PPU_SI_CLUS2CORE3 (0x56B40000)

#endif /* SCP_CSS_MMAP_H */
