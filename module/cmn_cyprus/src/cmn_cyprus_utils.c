/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions and utility functions for the CMN-Cyprus module.
 */

#include <internal/cmn_cyprus_reg.h>
#include <internal/cmn_cyprus_utils.h>

#include <mod_cmn_cyprus.h>

#include <fwk_assert.h>
#include <fwk_math.h>

#include <inttypes.h>
#include <stdint.h>

/* Node Info */
#define CMN_CYPRUS_NODE_INFO_ID             UINT64_C(0x00000000FFFF0000)
#define CMN_CYPRUS_NODE_INFO_ID_POS         16
#define CMN_CYPRUS_NODE_INFO_LOGICAL_ID     UINT64_C(0x0000FFFF00000000)
#define CMN_CYPRUS_NODE_INFO_LOGICAL_ID_POS 32

/* RNSAM HTG range comparision */
#define CMN_CYPRUS_RNSAM_UNIT_INFO_HTG_RANGE_COMP_EN_MASK UINT64_C(0x8000000)
#define CMN_CYPRUS_RNSAM_UNIT_INFO_HTG_RANGE_COMP_EN_POS  27

/* RNSAM RCOMP LSB */
#define CMN_CYPRUS_RNSAM_UNIT_INFO_HTG_RCOMP_LSB_PARAM_MASK     UINT64_C(0x1F)
#define CMN_CYPRUS_RNSAM_UNIT_INFO_NONHASH_RCOMP_LSB_PARAM_MASK UINT64_C(0x3E0)
#define CMN_CYPRUS_RNSAM_UNIT_INFO_NONHASH_RCOMP_LSB_PARAM_POS  5

static unsigned int get_rnsam_htg_rcomp_lsb_bit_pos(void *rnsam_reg)
{
    struct cmn_cyprus_rnsam_reg *rnsam = rnsam_reg;
    return (
        rnsam->UNIT_INFO[1] &
        CMN_CYPRUS_RNSAM_UNIT_INFO_HTG_RCOMP_LSB_PARAM_MASK);
}

static unsigned int get_rnsam_nonhash_rcomp_lsb_bit_pos(void *rnsam_reg)
{
    struct cmn_cyprus_rnsam_reg *rnsam = rnsam_reg;
    return (rnsam->UNIT_INFO[1] &
            CMN_CYPRUS_RNSAM_UNIT_INFO_NONHASH_RCOMP_LSB_PARAM_MASK) >>
        CMN_CYPRUS_RNSAM_UNIT_INFO_NONHASH_RCOMP_LSB_PARAM_POS;
}

unsigned int get_node_id(void *node_base)
{
    struct node_header *node = node_base;
    return (node->NODE_INFO & CMN_CYPRUS_NODE_INFO_ID) >>
        CMN_CYPRUS_NODE_INFO_ID_POS;
}

unsigned int get_node_logical_id(void *node_base)
{
    struct node_header *node = node_base;
    return (node->NODE_INFO & CMN_CYPRUS_NODE_INFO_LOGICAL_ID) >>
        CMN_CYPRUS_NODE_INFO_LOGICAL_ID_POS;
}

uint64_t sam_encode_region_size(uint64_t size)
{
    uint64_t blocks;
    uint64_t result;

    /* Size must be a multiple of SAM_GRANULARITY */
    fwk_assert((size % SAM_GRANULARITY) == 0);

    /* Size also must be a power of two */
    fwk_assert((size & (size - 1)) == 0);

    blocks = size / SAM_GRANULARITY;
    result = fwk_math_log2(blocks);

    return result;
}

bool get_rnsam_htg_range_comp_en_mode(void *rnsam_reg)
{
    struct cmn_cyprus_rnsam_reg *rnsam = rnsam_reg;
    return (rnsam->UNIT_INFO[0] &
            CMN_CYPRUS_RNSAM_UNIT_INFO_HTG_RANGE_COMP_EN_MASK) >>
        CMN_CYPRUS_RNSAM_UNIT_INFO_HTG_RANGE_COMP_EN_POS;
}

uint64_t get_rnsam_lsb_addr_mask(void *rnsam_reg, enum sam_type sam_type)
{
    uint64_t lsb_bit_pos;
    struct cmn_cyprus_rnsam_reg *rnsam = rnsam_reg;

    lsb_bit_pos = (sam_type == SAM_TYPE_NON_HASH_MEM_REGION) ?
        get_rnsam_nonhash_rcomp_lsb_bit_pos(rnsam) :
        get_rnsam_htg_rcomp_lsb_bit_pos(rnsam);

    return (1 << lsb_bit_pos) - 1;
}
