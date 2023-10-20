/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      CMN Cyprus driver module context structure.
 */

#ifndef INTERNAL_CMN_CYPRUS_CTX_H
#define INTERNAL_CMN_CYPRUS_CTX_H

#include <internal/cmn_cyprus_reg.h>
#include <internal/cmn_cyprus_utils.h>

#include <mod_cmn_cyprus.h>

#include <stdbool.h>
#include <stdint.h>

#define MAX_SCG_COUNT 4

struct hns_info {
    /* Pointer to the HN-S register */
    uintptr_t hns;
    /* Pointer to HN-S node position structure */
    struct node_pos node_pos;
    /* Pointer to the connected XP register */
    uintptr_t xp;
};

/* Pair of CCG Request Agent (CCG_RA) register and its node-id */
struct ccg_ra_reg_tuple {
    unsigned int node_id;
    struct cmn_cyprus_ccg_ra_reg *ccg_ra_reg;
};

/* Pair of CCG Request Agent (CCG_HA) register and its node-id */
struct ccg_ha_reg_tuple {
    unsigned int node_id;
    struct cmn_cyprus_ccg_ha_reg *ccg_ha_reg;
};

/* Pair of CCG Link Agent (CCLA) register and its node-id */
struct ccla_reg_tuple {
    unsigned int node_id;
    struct cmn_cyprus_ccla_reg *ccla_reg;
};

struct cmn_cyprus_ctx {
    /*! CMN Cyprus driver configuration data */
    const struct mod_cmn_cyprus_config_table *config_table;

    /*! Chip-specific configuration data */
    const struct mod_cmn_cyprus_config *config;

    /*! Base address of the CMN Cyprus configuration register */
    struct cmn_cyprus_cfgm_reg *root;

    /*! Count of HN-S nodes */
    unsigned int hns_count;

    /*! Pointer to the table of HN-S nodes */
    struct hns_info *hns_table;

    /*! Count of RN-SAM nodes */
    unsigned int rnsam_count;

    /*! List of RN-SAM node pointers */
    struct cmn_cyprus_rnsam_reg **rnsam_table;

    /*! Count of RN-D nodes */
    unsigned int rnd_count;

    /*! Count of RN-F nodes */
    unsigned int rnf_count;

    /*! Count of RN-I nodes */
    unsigned int rni_count;

    /*! Flag to indicate if the mesh has been discovered and initialized */
    bool is_initialized;

    /* Count of the I/O memory regions mapped in the RN SAM */
    unsigned int io_region_count;

    /* Count of the syscache memory regions(SCG) mapped in the RN SAM */
    unsigned int scg_count;

    /* Flags to indicate SCG init status */
    bool scg_enabled[MAX_SCG_COUNT];

    /* CCG_RA register and node_id pairs */
    struct ccg_ra_reg_tuple *ccg_ra_reg_table;

    /* CCG_HA register and node_id pairs */
    struct ccg_ha_reg_tuple *ccg_ha_reg_table;

    /* CCLA register and node_id pairs */
    struct ccla_reg_tuple *ccla_reg_table;

    /* Node count of CCG_RA, CCG_HA, CCLA nodes each. */
    size_t ccg_node_count;

    /* System Info module API */
    struct mod_system_info_get_info_api *system_info_api;

    /* Chip ID */
    uint8_t chip_id;

    /*
     * Logical device ID for the remote RN-F. Used when configuring the LDID
     * value in HN-S nodes during the CML programming.
     */
    unsigned int remote_rnf_ldid_value;

    /* Timer module API */
    struct mod_timer_api *timer_api;
};

#endif /* INTERNAL_CMN_CYPRUS_CTX_H */
