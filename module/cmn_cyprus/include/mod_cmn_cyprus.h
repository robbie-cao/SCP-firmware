/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_CMN_CYPRUS_H
#define MOD_CMN_CYPRUS_H

#include <fwk_id.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*!
 * \addtogroup GroupModules Modules
 * @{
 */

/*!
 * \defgroup GroupModuleCMN_CYPRUS CMN_CYPRUS
 *
 * \brief Arm Coherent Mesh Network (CMN) Cyprus driver module
 *
 * \details This module adds support for the CMN Cyprus interconnect
 * @{
 */

/*! Maximum CCG Protocol Links supported */
#define CMN_CYPRUS_MAX_CCG_PROTOCOL_LINKS 3

/*! Maximum RA SAM Address regions */
#define CMN_CYPRUS_MAX_RA_SAM_ADDR_REGION 8

/*!
 * \brief HN-S to SN-F memory striping modes
 */
enum mod_cmn_cyprus_hns_to_snf_mem_strip_mode {
    /* Direct mapping */
    MOD_CMN_CYPRUS_1_SN_MODE,
    /* 3-SN mode */
    MOD_CMN_CYPRUS_3_SN_MODE,
    /* 5-SN mode */
    MOD_CMN_CYPRUS_5_SN_MODE,
    /* 6-SN mode */
    MOD_CMN_CYPRUS_6_SN_MODE,
};

/*!
 * \brief Hierarchical hashing configuration
 */
struct mod_cmn_cyprus_hierarchical_hashing {
    /*!
     * \brief Number of HN-Ss per cluster.
     *
     * \note The value should not account for \ref
     * mod_cmn_cyprus_config.hns_cal_mode
     */
    unsigned int hns_cluster_count;

    /*!
     * \brief HN-S to SN-F hashing mode.
     */
    enum mod_cmn_cyprus_hns_to_snf_mem_strip_mode sn_mode;

    /*!
     * \brief Top PA address bit 0 to use for striping
     *
     * \note top_address_bit0 should match with the value in HN-S to SN-F strip
     * setting
     */
    unsigned int top_address_bit0;

    /*!
     * \brief Top PA address bit 1 to use for striping
     *
     * \note top_address_bit1 should match with the value in HN-S to SN-F strip
     * setting
     */
    unsigned int top_address_bit1;

    /*!
     * \brief Top PA address bit 2 to use for striping
     *
     * \note top_address_bit2 should match with the value in HN-S to SN-F strip
     * setting
     */
    unsigned int top_address_bit2;
};

/*!
 * \brief Coordinate (x, y, port number) of a node in the mesh
 */
struct node_pos {
    /*! x position of the node in the mesh */
    unsigned int pos_x;

    /*! y position of the node in the mesh */
    unsigned int pos_y;

    /*! port position of the node in the xp */
    unsigned int port_num;

    /*! Device position of the node in the xp port */
    unsigned int device_num;
};

/*!
 * \brief Memory region configuration type
 */
enum mod_cmn_cyprus_mem_region_type {
    /*! Input/Output region (serviced by dedicated HN-I and HN-D nodes) */
    MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,

    /*!
     * Region backed by the system cache (serviced by all HN-S nodes in the
     * system)
     */
    MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE,

    /*!
     * Sub region of the system cache for non-hashed access (serviced by
     * dedicated SN-F nodes).
     */
    MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE_SUB,

    /*!
     * Region used for CCG access (serviced by the CCRA nodes).
     */
    MOD_CMN_CYPRUS_REGION_TYPE_CCG,
};

/*!
 * \brief Isolated HN-S node descriptor
 */
struct isolated_hns_node_info {
    /*! Position of the isolated HN-S node */
    struct node_pos hns_pos;
    /*! Base address of the isolated HN-S node */
    uintptr_t hns_base;
};

/*!
 * \brief Memory region map descriptor
 */
struct mod_cmn_cyprus_mem_region_map {
    /*! Base address */
    uint64_t base;

    /*! Region size in bytes */
    uint64_t size;

    /*! Region configuration type */
    enum mod_cmn_cyprus_mem_region_type type;

    /*!
     * \brief Target node identifier
     *
     * \note Not used for \ref
     * mod_cmn_cyprus_mem_region_type.MOD_CMN_CYPRUS_REGION_TYPE_SYSCACHE
     * memory regions as it uses the pool of HN-S nodes available in the
     * system
     */
    unsigned int node_id;

    /*!
     * \brief HN-S start and end positions of a SCG/HTG
     *
     * \details Each SCG/HTG covers an address range and this address range can
     * be made to target a group of HN-Ss. These group of HN-Ss are typically
     * bound by an arbitrary rectangle/square in the mesh. To aid automatic
     * programming of the HN-Ss in SCG/HTG along with the discovery process,
     * each SCG/HTG takes hns_pos_start and hns_pos_end. HN-S nodes which are
     * bounded by this range will be assigned to the respective SCG/HTG. This
     * eliminates the process of manually looking at the mesh and assigning the
     * HN-S node ids to a SCG/HTG.
     *
     *                                        hns_pos_end
     *                                             xx
     *                                            xx
     *                                           xx
     *                    ┌─────────────────────xx
     *                    │                     │
     *                    │                     │
     *                    │                     │
     *                    │                     │
     *                    │    nth- SCG/HTG     │
     *                    │                     │
     *                    │                     │
     *                    │                     │
     *                    │                     │
     *                   xx─────────────────────┘
     *                  xx
     *                 xx
     *                xx
     *         hns_pos_start
     */

    /*!
     * \brief HN-S's bottom left node position
     *
     * \details \ref hns_pos_start is the HN-S's bottom left node position in
     * the rectangle covering the HN-Ss for a SCG/HTG
     *
     * \note To be used only with \ref
     * mod_cmn_cyprus_mem_region_type.MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE
     * memory regions.
     */
    struct node_pos hns_pos_start;

    /*!
     * \brief HN-S's top right node position
     *
     * \details \ref hns_pos_start is the HN-S's bottom left node position in
     * the rectangle covering the HN-Ss for a SCG/HTG
     *
     * \note To be used only with \ref
     * mod_cmn_cyprus_mem_region_type.MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE
     * memory regions.
     */
    struct node_pos hns_pos_end;
};

/*!
 * \brief Remote Memory region map descriptor for RA SAM programming
 */
struct mod_cmn_cyprus_ra_mem_region_map {
    /*! Base address */
    uint64_t base;

    /*! Region size in bytes */
    uint64_t size;

    /*! Target HAIDs of remote CCGs for this remote address region */
    unsigned int *remote_haid;
};

/*!
 * \brief CCG block descriptor
 *
 * \details Each CCG block can have up to eight remote memory map
 *      ::mod_cmn_cyprus_ra_mem_region_map descriptors and can have three links
 *      which can target different remote chips. User is expected to configure
 *      remote chip id ::mod_cmn_cyprus_ccg_config::remote_chip_id for
 *      link 0 as only link 0 is supported at the moment. The remote chip ID is
 *      used to assign unique request agent IDs across the chips. The overall
 *      structure of the descriptor is shown below:
 *
 *         +----------------------------------------------------------+
 *         | mod_cmn_cyprus_ccg_config<ldid>                          |
 *         |                                                          |
 *         |   HAID = haid                                            |
 *         +----------------------------------+-----------------------+
 *         | ra_mmap_table0                   |                       |
 *         |                                  |                       |
 *         | base..base+size --> remote_haid  |                       |
 *         |                                  |    remote_chip_id     |
 *         +----------------------------------+                       |
 *         | ra_mmap_table1                   |                       |
 *         |                                  |                       |
 *         | base..base+size --> remote_haid  |                       |
 *         |                                  +-----------------------+
 *         +----------------------------------+                       |
 *         | .                                |                       |
 *         | .                                |                       |
 *         | .                                |                       |
 *         | .                                |    remote_chip_id     |
 *         | .                                |                       |
 *         | .                                |                       |
 *         | .                                |                       |
 *         +----------------------------------+                       |
 *         | ra_mmap_table6                   +-----------------------+
 *         |                                  |                       |
 *         | base..base+size --> remote_haid  |                       |
 *         |                                  |                       |
 *         +----------------------------------+                       |
 *         | ra_mmap_table7                   |    remote_chip_id     |
 *         |                                  |                       |
 *         | base..base+size --> remote_haid  |                       |
 *         |                                  |                       |
 *         +----------------------------------+-----------------------+
 */
struct mod_cmn_cyprus_ccg_config {
    /*! Logical IDs of the CCG blocks to which this configuration applies */
    unsigned int *ldid;

    /*! Unique HAIDs in a multi-chip system. This has to be assigned manually */
    unsigned int *haid;

    /*! Number of remote RN Caching agents. */
    unsigned int remote_rnf_count;

    /*!
     * Table of remote region memory map entries. Used for programming the CCG
     * regions in RN SAM.
     */
    const struct mod_cmn_cyprus_mem_region_map
        remote_mmap_table[CMN_CYPRUS_MAX_RA_SAM_ADDR_REGION];

    /*!
     * Table of remote region memory map entries.
     * Used for programming the RA SAM.
     */
    const struct mod_cmn_cyprus_ra_mem_region_map
        ra_mmap_table[CMN_CYPRUS_MAX_RA_SAM_ADDR_REGION];

    /* ID of the remote chip that is connected to this CCG link */
    unsigned int remote_chip_id[CMN_CYPRUS_MAX_CCG_PROTOCOL_LINKS];

    /*! SMP Mode */
    bool smp_mode;

    /*!
     * \brief CCLA to CCLA direct connect mode
     *
     * \details CCG enables direct connection of CXS interface from the CCLA on
     * one CMN Cyprus to the CXS interface of the other. When such connection is
     * present, set this option to true in order to enable upper link layer to
     * upper link layer connection between CCLAs.
     */
    bool ull_to_ull_mode;

    /*! CCG Port Aggregation Mode */
    bool cpa_mode;

    /*! CCG Port Aggregation Group ID */
    uint8_t cpag_id;

    /*! Number of CCG Ports in the CPAG */
    uint8_t num_ports_in_cpag;
};

/*!
 * \brief CMN Cyprus configuration data
 */
struct mod_cmn_cyprus_config {
    /*! Peripheral base address */
    uintptr_t base;

    /*! Size along x-axis of the interconnect mesh */
    unsigned int mesh_size_x;

    /*! Size along y-axis of the interconnect mesh */
    unsigned int mesh_size_y;

    /*! Identifier of the clock that this device depends on */
    fwk_id_t clock_id;

    /*! Hierarchical hashing support */
    bool hierarchical_hashing_enable;

    /*! Hierarchical hashing configuration */
    struct mod_cmn_cyprus_hierarchical_hashing hierarchical_hashing_config;

    /*! Table of region memory map entries */
    const struct mod_cmn_cyprus_mem_region_map *mmap_table;

    /*! Number of entries in the \ref mmap_table */
    size_t mmap_count;

    /*!
     * \brief Table of SN-Fs used as targets for the HN-S nodes.
     *
     * \details Each entry of this table corresponds to a HN-S node in the
     *      system. The HN-S node's LDID is used as indices in this table.
     */
    const unsigned int *snf_table;

    /*! Number of entries in the \ref snf_table */
    size_t snf_count;

    /*!
     * \brief HN-S with CAL support flag
     *
     * \details When set to true, enables HN-S with CAL support. This flag will
     * be used only if HN-S is found to be connected to CAL (When connected to
     * a CAL port, node id of HN-S will be a odd number).
     *
     * \note Only CAL2 mode is supported at the moment.
     */
    bool hns_cal_mode;

    /*! Table of isolated HN-S nodes */
    struct isolated_hns_node_info *isolated_hns_table;

    /*! Number of entries in the \ref isolated_hns_table */
    unsigned int isolated_hns_count;

    /*! Table of CCG configuration */
    const struct mod_cmn_cyprus_ccg_config *ccg_config_table;

    /*!
     * \brief Number of entries in the
     *      ::mod_cmn_cyprus_config::ccg_config_table table.
     */
    const size_t ccg_table_count;

    /*! Address space size of the chip */
    uint64_t chip_addr_space;
};

/*!
 * \brief CMN Cyprus configuration table.
 */
struct mod_cmn_cyprus_config_table {
    /*! Table of chip-specific CMN Cyprus config data */
    struct mod_cmn_cyprus_config *chip_config_data;

    /*! Number of entries in the \ref chip_config_data */
    size_t chip_count;
};

/*!
 * \brief Module API indices
 */
enum mod_cmn_cyprus_api_idx {
    MOD_CMN_CYPRUS_API_IDX_MAP_IO_REGION,
    MOD_CMN_CYPRUS_API_COUNT,
};

/*!
 * \brief Module interface to manage mappings in RN SAM
 */
struct mod_cmn_cyprus_rnsam_memmap_api {
    /*!
     * \brief Program or update the given IO memory region in the RN SAM
     * \param base Base address of the IO memory region to be mapped
     * \param size Size of the IO memory region
     * \param node_id Target node ID
     *
     * \return FWK_SUCCESS The given IO memory region has been mapped
     * \return FWK_E_DATA Invalid region
     */
    int (*map_io_region)(uint64_t base, size_t size, uint32_t node_id);
};

/*!
 * @}
 */

/*!
 * @}
 */

#endif /* MOD_CMN_CYPRUS_H */
