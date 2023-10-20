/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Register definitions for the CMN-Cyprus module.
 */

#ifndef INTERNAL_CMN_CYPRUS_REG_H
#define INTERNAL_CMN_CYPRUS_REG_H

#include <fwk_macros.h>

#include <stdint.h>

/*!
 * Node header registers.
 */
// clang-format off
struct node_header {
    FWK_R uint64_t NODE_INFO;
          uint8_t  RESERVED0[0x80 - 0x8];
    FWK_R uint64_t CHILD_INFO;
          uint8_t  RESERVED1[0x100 - 0x88];
    FWK_R uint64_t CHILD_POINTER[256];
};
// clang-format on

/*!
 * Configuration manager registers.
 */
// clang-format off
struct cmn_cyprus_cfgm_reg {
    FWK_R   uint64_t  NODE_INFO;
    FWK_RW  uint64_t  PERIPH_ID[4];
    FWK_RW  uint64_t  COMPONENT_ID[2];
            uint8_t   RESERVED0[0x80 - 0x38];
    FWK_R   uint64_t  CHILD_INFO;
};
// clang-format on

/*!
 * Crosspoint (XP) registers.
 */
// clang-format off
struct cmn_cyprus_mxp_reg {
    FWK_R  uint64_t  NODE_INFO;
    FWK_R  uint64_t  PORT_CONNECT_INFO[6];
    FWK_R  uint64_t  PORT_CONNECT_INFO_EAST;
    FWK_R  uint64_t  PORT_CONNECT_INFO_NORTH;
           uint8_t   RESERVED0[0x80 - 0x48];
    FWK_R  uint64_t  CHILD_INFO;
           uint8_t   RESERVED1[0x100 - 0x88];
    FWK_R  uint64_t  CHILD_POINTER[32];
           uint8_t   RESERVED2[0xA70 - 0x200];
    FWK_RW uint64_t  PORT_DISABLE;
};
// clang-format on

#define HNS_RN_CLUSTER_MAX    128
#define HNS_RN_PHYIDS_REG_MAX 4

/*!
 * Fully Coherent Home Node (HN-S) registers.
 */
// clang-format off
struct cmn_cyprus_hns_reg {
    FWK_R   uint64_t  NODE_INFO;
            uint8_t   RESERVED0[0x80 - 0x8];
    FWK_R   uint64_t  CHILD_INFO;
            uint8_t   RESERVED1[0x900 - 0x88];
    FWK_R   uint64_t  UNIT_INFO[2];
            uint8_t   RESERVED2[0xD00 - 0x910];
    FWK_RW  uint64_t  SAM_CONTROL;
    FWK_RW  uint64_t  SAM_MEMREGION[2];
            uint8_t   RESERVED3[0xD28 - 0xD18];
    FWK_RW  uint64_t  SAM_CONTROL_2;
            uint8_t   RESERVED4[0xD38 - 0xD30];
    FWK_RW  uint64_t  SAM_MEMREGION_END_ADDR[2];
            uint8_t   RESERVED5[0xFB0 - 0xD48];
    FWK_RW  uint64_t  CML_PORT_AGGR_GRP_REG[2];
            uint8_t   RESERVED6[0x1900 - 0xFC0];
    FWK_RW  uint64_t  PPU_PWPR;
            uint8_t   RESERVED7[0x3C00 - 0x1908];
    FWK_RW  uint64_t
        HNS_RN_CLUSTER_PHYSID[HNS_RN_CLUSTER_MAX][HNS_RN_PHYIDS_REG_MAX];
};

/* Maximum Non Hash Mem Group regions in first group */
#define NON_HASH_MEM_REG_COUNT 24
/* Maximum Non Hash Mem Group regions in second group */
#define NON_HASH_MEM_REG_GRP2_COUNT 40

/*!
 * Request Node System Address Map (RN-SAM) registers.
 */
// clang-format off
struct cmn_cyprus_rnsam_reg {
    FWK_R   uint64_t  NODE_INFO;
            uint8_t   RESERVED0[0x80 - 0x8];
    FWK_R   uint64_t  CHILD_INFO;
            uint8_t   RESERVED1[0x900 - 0x88];
    FWK_R   uint64_t  UNIT_INFO[2];
            uint8_t   RESERVED2[0xC00 - 0x910];
    FWK_RW  uint64_t  NON_HASH_MEM_REGION[NON_HASH_MEM_REG_COUNT];
    FWK_RW  uint64_t  NON_HASH_MEM_REGION_CFG2[NON_HASH_MEM_REG_COUNT];
    FWK_RW  uint64_t  NON_HASH_TGT_NODEID[16];
    FWK_RW  uint64_t  SYS_CACHE_GRP_REGION[4];
    FWK_RW  uint64_t  HASHED_TGT_GRP_CFG1_REGION[4];
            uint8_t   RESERVED5[0xEA0 - 0xE40];
    FWK_RW  uint64_t  SYS_CACHE_GRP_HN_COUNT;
            uint8_t   RESERVED6[0xEB0 - 0xEA8];
    FWK_RW  uint64_t  SYS_CACHE_GRP_SN_ATTR[2];
            uint8_t   RESERVED7[0xF00 - 0xEC0];
    FWK_RW  uint64_t  SYS_CACHE_GRP_HN_NODEID[16];
            uint8_t   RESERVED8[0x1000 - 0xF80];
    FWK_RW  uint64_t  SYS_CACHE_GRP_SN_NODEID[32];
    FWK_RW  uint64_t  STATUS;
            uint64_t  GIC_MEM_REGION;
            uint8_t   RESERVED9[0x1120 - 0x1110];
    FWK_RW  uint64_t  SYS_CACHE_GRP_CAL_MODE;
    FWK_RW  uint64_t  HASHED_TARGET_GRP_CAL_MODE[3];
    FWK_RW  uint64_t  SYS_CACHE_GRP_SN_SAM_CFG[4];
            uint8_t   RESERVED10[0x11A0 - 0x1160];
    FWK_RW  uint64_t  CML_PORT_AGGR_MODE_CTRL_REG[4];
            uint8_t   RESERVED11[0x11F0 - 0x11C0];
    FWK_RW  uint64_t  CML_PORT_AGGR_GRP_REG[3];
    FWK_RW  uint64_t  CML_PORT_AGGR_CTRL_REG[16];
            uint8_t   RESERVED12[0x20C0 - 0x1288];
    FWK_RW  uint64_t  NON_HASH_MEM_REGION_GRP2[NON_HASH_MEM_REG_GRP2_COUNT];
            uint8_t   RESERVED13[0x24C0 - 0x2200];
    FWK_RW  uint64_t  NON_HASH_MEM_REGION_CFG2_GRP2[NON_HASH_MEM_REG_GRP2_COUNT];
            uint8_t   RESERVED14[0x2B00 - 0x2600];
    FWK_RW  uint64_t  CML_CPAG_BASE_INDX_GRP_REG[8];
            uint8_t   RESERVED15[0x3100 - 0x2B40];
    FWK_RW  uint64_t  HASHED_TGT_GRP_CFG2_REGION[32];
            uint8_t   RESERVED16[0x3400 - 0x3200];
    FWK_RW  uint64_t  HASHED_TARGET_GRP_HASH_CNTL[32];
};
// clang-format on

/*
 * CCG Gateway (CCG) protocol link control & status registers
 */
// clang-format off
struct ccg_link_regs {
    FWK_RW uint64_t CCG_CCPRTCL_LINK_CTRL;
    FWK_R uint64_t CCG_CCPRTCL_LINK_STATUS;
};
// clang-format on

/*
 * CCG Requesting Agent (RA) registers
 */
// clang-format off
struct cmn_cyprus_ccg_ra_reg {
    FWK_R  uint64_t CCG_RA_NODE_INFO;
           uint8_t  RESERVED0[0x80 - 0x8];
    FWK_R  uint64_t CCG_RA_CHILD_INFO;
           uint8_t  RESERVED1[0x900 - 0x88];
    FWK_R  uint64_t CCG_RA_UNIT_INFO;
           uint8_t  RESERVED2[0x980 - 0x908];
    FWK_RW uint64_t CCG_RA_SCR;
           uint8_t  RESERVED3[0xA00 - 0x988];
    FWK_RW uint64_t CCG_RA_CFG_CTRL;
    FWK_RW uint64_t CCG_RA_AUX_CTRL;
           uint8_t  RESERVED4[0xC00 - 0xA10];
    FWK_RW uint64_t CCG_RA_SAM_ADDR_REGION_REG[8];
           uint8_t  RESERVED5[0xD00 - 0xC40];
    FWK_RW uint64_t CCG_RA_AGENTID_TO_LINKID_VAL;
           uint8_t  RESERVED6[0xD10 - 0xD08];
    FWK_RW uint64_t CCG_RA_AGENTID_TO_LINKID_REG[8];
           uint8_t  RESERVED7[0xE00 - 0xD50];
    FWK_RW uint64_t CCG_RA_RNI_LDID_TO_EXP_RAID_REG[10];
           uint8_t  RESERVED8[0xF00 - 0xE50];
    FWK_RW uint64_t CCG_RA_RND_LDID_TO_EXP_RAID_REG[10];
           uint8_t  RESERVED9[0x1000 - 0xF50];
    FWK_RW uint64_t CCG_RA_RNF_LDID_TO_EXP_RAID_REG[128];
           uint8_t  RESERVED10[0x1680 - 0x1400];
    FWK_RW uint64_t CCG_RA_RNF_LDID_TO_OVRD_LDID_REG[128];
           uint8_t  RESERVED11[0x4000 - 0x1A80];
           struct ccg_link_regs LINK_REGS[3];
           uint8_t  RESERVED12[0xD900 - 0x4030];
    FWK_RW uint64_t CCG_RA_PMU_EVENT_SEL;
};
// clang-format on

/*
 * CCG Gateway (CCG) Home Agent (HA) registers
 */
// clang-format off
struct cmn_cyprus_ccg_ha_reg {
    FWK_R  uint64_t CCG_HA_NODE_INFO;
    FWK_RW uint64_t CCG_HA_ID;
           uint8_t  RESERVED0[0x80 - 0x10];
    FWK_R  uint64_t CCG_HA_CHILD_INFO;
           uint8_t  RESERVED1[0x900 - 0x88];
    FWK_R  uint64_t CCG_HA_UNIT_INFO[3];
           uint8_t  RESERVED2[0x980 - 0x918];
    FWK_RW uint64_t CCG_HA_SCR;
           uint8_t  RESERVED3[0xA00 - 0x988];
    FWK_RW uint64_t CCG_HA_CFG_CTRL;
    FWK_RW uint64_t CCG_HA_AUX_CTRL;
           uint8_t  RESERVED4[0xC00 - 0xA10];
    FWK_RW uint64_t CCG_HA_RNF_EXP_RAID_TO_LDID_REG[256];
           uint8_t  RESERVED5[0x1900 - 0x1400];
           struct ccg_link_regs LINK_REGS[3];
           uint8_t  RESERVED6[0x1C00 - 0x1930];
    FWK_RW uint64_t CCG_HA_AGENTID_TO_LINKID_REG[8];
           uint8_t  RESERVED7[0x1CF8 - 0x1C40];
    FWK_RW uint64_t CCG_HA_AGENTID_TO_LINKID_VAL;
           uint8_t  RESERVED8[0xD900 - 0x1D00];
    FWK_RW uint64_t CCG_HA_PMU_EVENT_SEL;
};
// clang-format on

/*
 * CCG Gateway (CCG) Link Agent (LA) registers
 */
// clang-format off
struct cmn_cyprus_ccla_reg {
    FWK_R  uint64_t CCLA_NODE_INFO;
           uint8_t  RESERVED0[0x80 - 0x8];
    FWK_R  uint64_t CCLA_CHILD_INFO;
           uint8_t  RESERVED1[0x910 - 0x88];
    FWK_R  uint64_t CCLA_UNIT_INFO;
           uint8_t  RESERVED2[0x980 - 0x918];
    FWK_RW uint64_t CCLA_SCR;
    FWK_RW uint64_t CCLA_RCR;
           uint8_t  RESERVED3[0xB00 - 0x990];
    FWK_RW uint64_t CCLA_CFG_CTL;
    FWK_RW uint64_t CCLA_AUX_CTRL;
           uint8_t  RESERVED4[0xC00 - 0xB10];
    FWK_R  uint64_t CCLA_CCIX_PROP_CAPABILITIES;
    FWK_RW uint64_t CCLA_CXS_ATTR_CAPABILITIES;
           uint8_t  RESERVED5[0xD28 - 0xC10];
    FWK_RW uint64_t CCLA_ERR_AGENT_ID;
    FWK_RW uint64_t CCLA_AGENTID_TO_PORTID_REG[8];
    FWK_RW uint64_t CCLA_AGENTID_TO_PORTID_VAL;
    FWK_RW uint64_t CCLA_PORTFWD_EN;
    FWK_R  uint64_t CCLA_PORTFWD_STATUS;
           uint8_t  RESERVED6[0xE00 - 0xD88];
    FWK_RW uint64_t CCLA_CXL_LINK_RX_CREDIT_CTL;
    FWK_R  uint64_t CCLA_CXL_LINK_RX_CREDIT_RETURN_STAT;
    FWK_R  uint64_t CCLA_CXL_LINK_TX_CREDIT_STAT;
    FWK_RW uint64_t CCLA_CXL_LINK_LAYER_DEFEATURE;
    FWK_RW uint64_t CCLA_ULL_CTL;
    FWK_R  uint64_t CCLA_ULL_STATUS;
    FWK_RW uint64_t CCLA_CXL_LL_ERRINJECT_CTL;
    FWK_R  uint64_t CCLA_CXL_LL_ERRINJECT_STAT;
           uint8_t  RESERVED7[0xD908 - 0xE40];
    FWK_RW uint64_t CCLA_PMU_EVENT_SEL;
           uint8_t  RESERVED8[0xE000 - 0xD910];
    FWK_R  uint64_t CCLA_ERRFR;
    FWK_RW uint64_t CCLA_ERRCTLR;
    FWK_W  uint64_t CCLA_ERRSTATUS;
    FWK_RW uint64_t CCLA_ERRADDR;
           uint8_t  RESERVED9;
    FWK_RW uint64_t CCLA_ERRMISC1;
           uint8_t  RESERVED10[0xE040 - 0xE030];
    FWK_R  uint64_t CCLA_ERRFR_NS;
    FWK_RW uint64_t CCLA_ERRCTLR_NS;
    FWK_W  uint64_t CCLA_ERRSTATUS_NS;
    FWK_RW uint64_t CCLA_ERRADDR_NS;
           uint8_t  RESERVED11;
    FWK_RW uint64_t CCLA_ERRMISC1_NS;
};
// clang-format on

#endif /* INTERNAL_CMN_CYPRUS_REG_H */
