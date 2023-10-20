/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef IO_MACRO_LAYOUT
#define IO_MACRO_LAYOUT

enum asni_domain_id {
    ASNI_CMN,
    ASNI_PCIEX1_0,
    ASNI_PCIEX2_1,
    ASNI_DOMAIN_MAX_ID
};

enum amni_domain_id {
    AMNI_CMN,
    AMNI_INLINE_IO_CFG,
    AMNI_INLINE_IO,
    AMNI_PCIEX1_0_CFG,
    AMNI_PCIEX1_0,
    AMNI_PCIEX2_0_CFG,
    AMNI_PCIEX2_0,
    AMNI_PCIEX2_1_CFG,
    AMNI_PCIEX2_1,
    AMNI_PCIEX4_0_CFG,
    AMNI_PCIEX4_0,
    AMNI_PCIEX8_0_CFG,
    AMNI_PCIEX8_0,
    AMNI_PMNI_CTRL_REG_APB,
    AMNI_PMNI_TCU_APB,
    AMNI_DOMAIN_MAX_ID
};
#endif
