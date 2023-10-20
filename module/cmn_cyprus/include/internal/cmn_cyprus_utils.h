/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions and utility functions for the CMN-Cyprus module.
 */

#ifndef INTERNAL_CMN_CYPRUS_UTILS_H
#define INTERNAL_CMN_CYPRUS_UTILS_H

#include <stdbool.h>
#include <stdint.h>

#define MOD_NAME "[CMN_CYPRUS] "

/* SAM Granularity of RN-SAM and HN-F SAM */
#define SAM_GRANULARITY (64 * FWK_MIB)

/*
 * Used by RNSAM and HNSAM CPA registers
 *
 * CPA - CCIX Port Aggregation
 * PAG - Port Aggregation Group
 * GRPID - Group ID
 */
#define CMN_PORT_AGGR_GRP_PAG_TGTID_PER_GROUP       5
#define CMN_PORT_AGGR_GRP_PAG_TGTID_WIDTH           12
#define CMN_PORT_AGGR_GRP_PAG_TGTID_WIDTH_PER_GROUP 60

/*!
 * RN SAM SAM region types.
 */
enum sam_type {
    /*! RN SAM Non-Hashed memory region */
    SAM_TYPE_NON_HASH_MEM_REGION,
    /*! RN SAM System cache backed(hashed) memory region */
    SAM_TYPE_SYS_CACHE_GRP_REGION,
};

/*
 * Retrieve the physical identifier of a node from its hardware node descriptor.
 * This identifier encodes the node's position in the mesh.
 *
 * Note: Multiple node descriptors can share the same identifier if they are
 * related to the same device node in the mesh.
 *
 * \param node_base Pointer to the node descriptor
 *      \pre The node pointer must be valid
 *
 * \return Node's physical identifier
 */
unsigned int get_node_id(void *node_base);

/*
 * Retrieve the logical identifier of a node from its hardware node descriptor.
 * This is an unique identifier (index) among nodes of the same type in the
 * system.
 *
 * \param node_base Pointer to the node base address
 *      \pre The node pointer must be valid
 *
 * \return An integer representing the node's logical identifier
 */
unsigned int get_node_logical_id(void *node_base);

/*
 * Convert a memory region size into a size format used by the CMN-CYPRUS
 * registers. The format is the binary logarithm of the memory region size
 * represented as blocks multiple of the CMN-CYPRUS's granularity:
 * n =  log2(size / SAM_GRANULARITY)
 *
 * \param size Memory region size to be converted
 *      \pre size must be a multiple of SAM_GRANULARITY
 *
 * \return log2(size / SAM_GRANULARITY)
 */
uint64_t sam_encode_region_size(uint64_t size);

/*
 * Get HTG range comparision mode status.
 *
 * \param rnsam_reg Pointer to the rnsam register.
 *      \pre The rnsam register pointer must be valid.
 *
 * \return True if range comparision mode is enabled for HTGs.
 *         False otherwise.
 */
bool get_rnsam_htg_range_comp_en_mode(void *rnsam_reg);

/*
 * Get RN SAM LSB address mask.
 *
 * \param rnsam_reg Pointer to the rnsam register.
 *      \pre The rnsam register pointer must be valid.
 * \param sam_type RN SAM region type
 *
 * \return RN SAM LSB address mask.
 */
uint64_t get_rnsam_lsb_addr_mask(void *rnsam_reg, enum sam_type sam_type);

#endif /* INTERNAL_CMN_CYPRUS_UTILS_H */
