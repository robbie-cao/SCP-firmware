/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'cmn_cyprus'.
 */

#include "cmn_node_id.h"
#include "platform_core.h"
#include "scp_clock.h"
#include "scp_css_mmap.h"
#include "scp_exp_mmap.h"

#include <mod_cmn_cyprus.h>

#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if (PLATFORM_VARIANT == 2)

/* CCG ports available on the variant 2 */
enum rdfremontcfg2_cmn_cyprus_ccg_port {
    CCG_PORT_0,
    CCG_PORT_1,
    CCG_PORT_2,
    CCG_PORT_3,
    CCG_PORT_4,
    CCG_PORT_5,
    CCG_PORT_6,
    CCG_PORT_7,
    CCG_PORT_8,
    CCG_PORT_9,
    CCG_PER_CHIP,
};

/* Total RN-Fs (Poseidon CPUs) per chips for variant 2 */
#define RNF_PER_CHIP_CFG2 32

static const unsigned int snf_table[] = {
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 0  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 1  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 2  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 3  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 4  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 5  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 6  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 7  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 8  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 9  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 10  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 11  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 12  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 13  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 14  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 15  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 16  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 17  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 18  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 19  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 20  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 21  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 22  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 23  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 24  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 25  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 26  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 27  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 28  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 29  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 30  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 31  */
};
#else
static const unsigned int snf_table[] = {
    MEM_CNTRL0_ID, /* Maps to HN-F logical node 0  */
    MEM_CNTRL0_ID, /* Maps to HN-F logical node 1  */
    MEM_CNTRL0_ID, /* Maps to HN-F logical node 2  */
    MEM_CNTRL0_ID, /* Maps to HN-F logical node 3  */
    MEM_CNTRL1_ID, /* Maps to HN-F logical node 4  */
    MEM_CNTRL1_ID, /* Maps to HN-F logical node 5  */
    MEM_CNTRL1_ID, /* Maps to HN-F logical node 6  */
    MEM_CNTRL1_ID, /* Maps to HN-F logical node 7  */
#if (PLATFORM_VARIANT == 0)
    MEM_CNTRL2_ID, /* Maps to HN-F logical node 8  */
    MEM_CNTRL2_ID, /* Maps to HN-F logical node 9  */
    MEM_CNTRL2_ID, /* Maps to HN-F logical node 10  */
    MEM_CNTRL2_ID, /* Maps to HN-F logical node 11  */
    MEM_CNTRL3_ID, /* Maps to HN-F logical node 12  */
    MEM_CNTRL3_ID, /* Maps to HN-F logical node 13  */
    MEM_CNTRL3_ID, /* Maps to HN-F logical node 14  */
    MEM_CNTRL3_ID, /* Maps to HN-F logical node 15  */
    MEM_CNTRL4_ID, /* Maps to HN-F logical node 16  */
    MEM_CNTRL4_ID, /* Maps to HN-F logical node 17  */
    MEM_CNTRL4_ID, /* Maps to HN-F logical node 18  */
    MEM_CNTRL4_ID, /* Maps to HN-F logical node 19  */
    MEM_CNTRL5_ID, /* Maps to HN-F logical node 20  */
    MEM_CNTRL5_ID, /* Maps to HN-F logical node 21  */
    MEM_CNTRL5_ID, /* Maps to HN-F logical node 22  */
    MEM_CNTRL5_ID, /* Maps to HN-F logical node 23  */
    MEM_CNTRL6_ID, /* Maps to HN-F logical node 24  */
    MEM_CNTRL6_ID, /* Maps to HN-F logical node 25  */
    MEM_CNTRL6_ID, /* Maps to HN-F logical node 26  */
    MEM_CNTRL6_ID, /* Maps to HN-F logical node 27  */
    MEM_CNTRL7_ID, /* Maps to HN-F logical node 28  */
    MEM_CNTRL7_ID, /* Maps to HN-F logical node 29  */
    MEM_CNTRL7_ID, /* Maps to HN-F logical node 30  */
    MEM_CNTRL7_ID, /* Maps to HN-F logical node 31  */
#endif
};
#endif

static const struct mod_cmn_cyprus_mem_region_map mmap[] = {
    {
        /*
         * System cache backed region
         * Map: 0x0000_0000_0000 - 0x03FF_FFFF_FFFF (4 TiB)
         */
        .base = UINT64_C(0x000000000000),
        .size = UINT64_C(256) * FWK_TIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE,
        .hns_pos_start = { 0, 0, 0 },
        .hns_pos_end = { MESH_SIZE_X - 1, MESH_SIZE_Y - 1, 1 },
    },
    {
        /*
         * Shared SRAM
         * Map: 0x0000_0000_0000 - 0x0000_07FF_FFFF (128 MB)
         */
        .base = UINT64_C(0x000000000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE_SUB,
        .node_id = NODE_ID_SBSX,
    },
    {
        /*
         * Boot Flash
         * Map: 0x00_0800_0000 - 0x00_0FFF_FFFF (128 MB)
         */
        .base = UINT64_C(0x0008000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
#if (PLATFORM_VARIANT == 0 || PLATFORM_VARIANT == 2)
        .node_id = NODE_ID_HNT1,
#else
        .node_id = NODE_ID_HNT0,
#endif
    },
    {
        /*
         * Peripherals
         * Map: 0x00_1000_0000 - 0x00_2EFF_FFFF (496 MB)
         */
        .base = UINT64_C(0x0010000000),
        .size = UINT64_C(496) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * Shared SRAM
         * Map: 0x00_2F00_0000 - 0x00_2F3F_FFFF (4 MB)
         */
        .base = UINT64_C(0x002F000000),
        .size = UINT64_C(4) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE_SUB,
        .node_id = NODE_ID_SBSX,
    },
    {
        /*
         * Peripherals
         * Map: 0x00_2F40_0000 - 0x00_5FFF_FFFF (780 MB)
         */
        .base = UINT64_C(0x002F400000),
        .size = UINT64_C(780) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * CMN_CYPRUS GPV
         * Map: 0x01_0000_0000 - 0x01_3FFF_FFFF (1 GB)
         */
        .base = UINT64_C(0x0100000000),
        .size = UINT64_C(1) * FWK_GIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * Cluster Utility Memory region
         * Map: 0x2_0000_0000 - 0x2_3FFF_FFFF (1 GB)
         */
        .base = UINT64_C(0x200000000),
        .size = UINT64_C(1) * FWK_GIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * Peripherals - Memory Controller
         * Map: 0x2_4000_0000 - 0x2_4FFF_FFFF (256 MB)
         */
        .base = UINT64_C(0x240000000),
        .size = UINT64_C(256) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * Peripherals, NCI GPV Memory Map 0
         * Map: 0x02_8000_0000 - 0x02_87FF_FFFF (128 MB)
         */
        .base = UINT64_C(0x0280000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = IOVB_NODE_ID0,
    },
#if (PLATFORM_VARIANT == 0)
    {
        /*
         * Peripherals, NCI GPV Memory Map 1
         * Map: 0x02_8800_0000 - 0x02_8FFF_FFFF (128 MB)
         */
        .base = UINT64_C(0x0288000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = IOVB_NODE_ID1,
    },
    {
        /*
         * Peripherals, NCI GPV Memory Map 2
         * Map: 0x02_9000_0000 - 0x02_97FF_FFFF (128 MB)
         */
        .base = UINT64_C(0x0290000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = IOVB_NODE_ID2,
    },
    {
        /*
         * Peripherals, NCI GPV Memory Map 3
         * Map: 0x02_9800_0000 - 0x02_9FFF_FFFF (128 MB)
         */
        .base = UINT64_C(0x0298000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = IOVB_NODE_ID3,
    },
    {
        /*
         * Peripherals, NCI GPV Memory Map 4
         * Map: 0x02_A000_0000 - 0x02_A7FF_FFFF (128 MB)
         */
        .base = UINT64_C(0x02A0000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = IOVB_NODE_ID4,
    },
#endif
    {
        /*
         * GPC_SMMU region
         * Map: 0x03_0000_0000 - 0x03_07FF_FFFF (128 MB)
         */
        .base = UINT64_C(0x300000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * Non Secure NOR Flash 0/1
         * Map: 0x06_0000_0000 - 0x06_07FF_FFFF (128 MB)
         */
        .base = UINT64_C(0x0600000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * Ethernet Controller PL91x
         * Map: 0x06_0C00_0000 - 0x06_0FFF_FFFF (64 MB)
         */
        .base = UINT64_C(0x060C000000),
        .size = UINT64_C(64) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
};

#if (PLATFORM_VARIANT == 2)
/* Multichip related configuration data */
/*
 * Cross chip CCG connections between the chips:
 *
 *  +----------------------------------+             +-------------------------------+
 *  |                                  |             |                               |
 *  |                              CCG6+-------------+CCG6                           |
 *  |                                  |             |                           CCG4|
 *  |CCG4                          CCG7+-------------+CCG7                           |
 *  |             CHIP 0               |             |             CHIP 2            |
 *  |                              CCG8+----+  +-----+CCG8                       CCG5|
 *  |CCG5                              |    |  |     |                               |
 *  |                              CCG9+-+  |  |  +--+CCG9                           |
 *  |                                  | |  |  |  |  |                               |
 *  |   CCG0    CCG1    CCG2    CCG3   | |  |  |  |  |   CCG0   CCG1   CCG2   CCG3   |
 *  +----+-------+-------+-------+-----+ |  |  |  |  +-----+------+------+------+----+
 *       |       |       |       |       |  |  |  |        |      |      |      |
 *       |       |       |       |       |  |  |  |        |      |      |      |
 *  +----+-------+-------+-------+-----+ |  |  |  |  +-----+------+------+------+----+
 *  |   CCG3    CCG2    CCG1    CCG0   | |  |  |  |  |    CCG3  CCG2   CCG1    CCG0  |
 *  |                                  | |  |  |  |  |                               |
 *  |                              CCG4+-|--|--+  |  |                               |
 *  |                                  | |  |     |  |                               |
 *  |                              CCG5+-|--|-----+  |                               |
 *  |                                  | |  |        |                               |
 *  |CCG8                              | |  +--------+CCG4                       CCG8|
 *  |             CHIP 1               | |           |             CHIP 3            |
 *  |                                  | +-----------+CCG5                           |
 *  |CCG9                              |             |                           CCG9|
 *  |                              CCG6+-------------+CCG6                           |
 *  |                                  |             |                               |
 *  |                              CCG7+-------------+CCG7                           |
 *  |                                  |             |                               |
 *  +----------------------------------+             +-------------------------------+
 */
/* Chip-0 Config data */
// CHIP 0 --> CHIP 1
unsigned int chip0_cml0_ldid[] = {
    CCG_PORT_0,
    CCG_PORT_1
};
unsigned int chip0_cml0_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_0) + CCG_PORT_0,
    (CCG_PER_CHIP * PLATFORM_CHIP_0) + CCG_PORT_1,
};
unsigned int chip0_cml0_remote_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_1) + CCG_PORT_3,
    (CCG_PER_CHIP * PLATFORM_CHIP_1) + CCG_PORT_2
};
// CHIP 0 --> CHIP 2
unsigned int chip0_cml1_ldid[] = {
    CCG_PORT_6,
    CCG_PORT_7,
};
unsigned int chip0_cml1_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_0) + CCG_PORT_6,
    (CCG_PER_CHIP * PLATFORM_CHIP_0) + CCG_PORT_7,
};
unsigned int chip0_cml1_remote_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_2) + CCG_PORT_6,
    (CCG_PER_CHIP * PLATFORM_CHIP_2) + CCG_PORT_7,
};
// CHIP 0 --> CHIP 3
unsigned int chip0_cml2_ldid[] = {
    CCG_PORT_8,
    CCG_PORT_9,
};
unsigned int chip0_cml2_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_0) + CCG_PORT_8,
    (CCG_PER_CHIP * PLATFORM_CHIP_0) + CCG_PORT_9,
};
unsigned int chip0_cml2_remote_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_3) + CCG_PORT_4,
    (CCG_PER_CHIP * PLATFORM_CHIP_3) + CCG_PORT_5,
};

static const struct mod_cmn_cyprus_ccg_config ccg_config_table_chip_0[] = {
    {
        .ldid = chip0_cml0_ldid,
        .haid =  chip0_cml0_haid,
        .remote_rnf_count = RNF_PER_CHIP_CFG2 * (PLATFORM_CHIP_COUNT - 1),
        .remote_mmap_table = {
            {
                .base = UINT64_C(0x1000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            {
                .base = UINT64_C(0x100000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            { 0 }
        },
        .ra_mmap_table = {
            {
                .base = UINT64_C(0x1000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .remote_haid = chip0_cml0_remote_haid,
            },
            {
                .base = UINT64_C(0x100000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .remote_haid = chip0_cml0_remote_haid,
            },
            { 0 }
        },
        .remote_chip_id = {
            [0] = PLATFORM_CHIP_1,
        },
        .smp_mode = true,
        .ull_to_ull_mode = true,
        .cpa_mode = true,
        .cpag_id = 0,
        .num_ports_in_cpag = 2,
    },
    {
        .ldid =  chip0_cml1_ldid,
        .haid =  chip0_cml1_haid,
        .remote_rnf_count = RNF_PER_CHIP_CFG2 * (PLATFORM_CHIP_COUNT - 1),
        .remote_mmap_table = {
            {
                .base = UINT64_C(0x2000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            {
                .base = UINT64_C(0x200000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            { 0 }
        },
        .ra_mmap_table = {
            {
                .base = UINT64_C(0x2000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .remote_haid = chip0_cml1_remote_haid,
            },
            {
                .base = UINT64_C(0x200000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .remote_haid = chip0_cml1_remote_haid,
            },
            { 0 }
        },
        .remote_chip_id = {
            [0] = PLATFORM_CHIP_2,
        },
        .smp_mode = true,
        .ull_to_ull_mode = true,
        .cpa_mode = true,
        .cpag_id = 1,
        .num_ports_in_cpag = 2,
    },
    {
        .ldid =  chip0_cml2_ldid,
        .haid =  chip0_cml2_haid,
        .remote_rnf_count = RNF_PER_CHIP_CFG2 * (PLATFORM_CHIP_COUNT - 1),
        .remote_mmap_table = {
            {
                .base = UINT64_C(0x3000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            {
                .base = UINT64_C(0x300000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            { 0 }
        },
        .ra_mmap_table = {
            {
                .base = UINT64_C(0x3000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .remote_haid = chip0_cml2_remote_haid,
            },
            {
                .base = UINT64_C(0x300000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .remote_haid = chip0_cml2_remote_haid,
            },
            { 0 }
        },
        .remote_chip_id = {
            [0] = PLATFORM_CHIP_3,
        },
        .smp_mode = true,
        .ull_to_ull_mode = true,
        .cpa_mode = true,
        .cpag_id = 2,
        .num_ports_in_cpag = 2,
    },
};

/* Chip-1 Config data */
// CHIP 1 --> CHIP 0
unsigned int chip1_cml0_ldid[] = {
    CCG_PORT_3,
    CCG_PORT_2,
};
unsigned int chip1_cml0_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_1) + CCG_PORT_3,
    (CCG_PER_CHIP * PLATFORM_CHIP_1) + CCG_PORT_2,
};
unsigned int chip1_cml0_remote_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_0) + CCG_PORT_0,
    (CCG_PER_CHIP * PLATFORM_CHIP_0) + CCG_PORT_1,
};
// CHIP 1 --> CHIP 2
unsigned int chip1_cml1_ldid[] = {
    CCG_PORT_4,
    CCG_PORT_5,
};
unsigned int chip1_cml1_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_1) + CCG_PORT_4,
    (CCG_PER_CHIP * PLATFORM_CHIP_1) + CCG_PORT_5,
};
unsigned int chip1_cml1_remote_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_2) + CCG_PORT_8,
    (CCG_PER_CHIP * PLATFORM_CHIP_2) + CCG_PORT_9,
};
// CHIP 1 --> CHIP 3
unsigned int chip1_cml2_ldid[] = {
    CCG_PORT_6,
    CCG_PORT_7,
};
unsigned int chip1_cml2_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_1) + CCG_PORT_6,
    (CCG_PER_CHIP * PLATFORM_CHIP_1) + CCG_PORT_7,
};
unsigned int chip1_cml2_remote_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_3) + CCG_PORT_6,
    (CCG_PER_CHIP * PLATFORM_CHIP_3) + CCG_PORT_7,
};

static const struct mod_cmn_cyprus_ccg_config ccg_config_table_chip_1[] = {
    // CHIP 1 --> CHIP 0
    {
        .ldid =  chip1_cml0_ldid,
        .haid =  chip1_cml0_haid,
        .remote_rnf_count = RNF_PER_CHIP_CFG2 * (PLATFORM_CHIP_COUNT - 1),
        .remote_mmap_table = {
            {
                .base = UINT64_C(0x000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            {
                .base = UINT64_C(0x8080000000),
                .size = UINT64_C(16) * FWK_TIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            { 0 }
        },
        .ra_mmap_table = {
            {
                .base = UINT64_C(0x00000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .remote_haid = chip1_cml0_remote_haid,
            },
            {
                .base = UINT64_C(0x8080000000),
                .size = UINT64_C(16) * FWK_TIB,
                .remote_haid = chip1_cml0_remote_haid,
            },
            { 0 }
        },
        .remote_chip_id = {
            [0] = PLATFORM_CHIP_0,
        },
        .smp_mode = true,
        .ull_to_ull_mode = true,
        .cpa_mode = true,
        .cpag_id = 0,
        .num_ports_in_cpag = 2,
    },
    // CHIP 1 --> CHIP 2
    {
        .ldid =  chip1_cml1_ldid,
        .haid =  chip1_cml1_haid,
        .remote_rnf_count = RNF_PER_CHIP_CFG2 * (PLATFORM_CHIP_COUNT - 1),
        .remote_mmap_table = {
            {
                .base = UINT64_C(0x2000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            {
                .base = UINT64_C(0x200000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            { 0 }
        },
        .ra_mmap_table = {
            {
                .base = UINT64_C(0x2000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .remote_haid = chip1_cml1_remote_haid,
            },
            {
                .base = UINT64_C(0x200000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .remote_haid = chip1_cml1_remote_haid,
            },
            { 0 }
        },
        .remote_chip_id = {
            [0] = PLATFORM_CHIP_2,
        },
        .smp_mode = true,
        .ull_to_ull_mode = true,
        .cpa_mode = true,
        .cpag_id = 1,
        .num_ports_in_cpag = 2,
    },
    // CHIP 1 --> CHIP 3
    {
        .ldid =  chip1_cml2_ldid,
        .haid =  chip1_cml2_haid,
        .remote_rnf_count = RNF_PER_CHIP_CFG2 * (PLATFORM_CHIP_COUNT - 1),
        .remote_mmap_table = {
            {
                .base = UINT64_C(0x3000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            {
                .base = UINT64_C(0x300000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            { 0 }
        },
        .ra_mmap_table = {
            {
                .base = UINT64_C(0x3000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .remote_haid = chip1_cml2_remote_haid,
            },
            {
                .base = UINT64_C(0x300000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .remote_haid = chip1_cml2_remote_haid,
            },
            { 0 }
        },
        .remote_chip_id = {
            [0] = PLATFORM_CHIP_3,
        },
        .smp_mode = true,
        .ull_to_ull_mode = true,
        .cpa_mode = true,
        .cpag_id = 2,
        .num_ports_in_cpag = 2,
    },
};

/* Chip-2 Config data */
// CHIP 2 --> CHIP 0
unsigned int chip2_cml0_ldid[] = {
    CCG_PORT_6,
    CCG_PORT_7,
};
unsigned int chip2_cml0_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_2) + CCG_PORT_6,
    (CCG_PER_CHIP * PLATFORM_CHIP_2) + CCG_PORT_7,
};
unsigned int chip2_cml0_remote_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_0) + CCG_PORT_6,
    (CCG_PER_CHIP * PLATFORM_CHIP_0) + CCG_PORT_7,
};
// CHIP 2 --> CHIP 1
unsigned int chip2_cml1_ldid[] = {
    CCG_PORT_8,
    CCG_PORT_9,
};
unsigned int chip2_cml1_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_2) + CCG_PORT_8,
    (CCG_PER_CHIP * PLATFORM_CHIP_2) + CCG_PORT_9,
};
unsigned int chip2_cml1_remote_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_1) + CCG_PORT_4,
    (CCG_PER_CHIP * PLATFORM_CHIP_1) + CCG_PORT_5,
};
// CHIP 2 --> CHIP 3
unsigned int chip2_cml2_ldid[] = {
    CCG_PORT_0,
    CCG_PORT_1,
};
unsigned int chip2_cml2_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_2) + CCG_PORT_0,
    (CCG_PER_CHIP * PLATFORM_CHIP_2) + CCG_PORT_1,
};
unsigned int chip2_cml2_remote_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_3) + CCG_PORT_3,
    (CCG_PER_CHIP * PLATFORM_CHIP_3) + CCG_PORT_2,
};

static const struct mod_cmn_cyprus_ccg_config ccg_config_table_chip_2[] = {
    // CHIP 2 --> CHIP 0
    {
        .ldid =  chip2_cml0_ldid,
        .haid =  chip2_cml0_haid,
        .remote_rnf_count = RNF_PER_CHIP_CFG2 * (PLATFORM_CHIP_COUNT - 1),
        .remote_mmap_table = {
            {
                .base = UINT64_C(0x000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            {
                .base = UINT64_C(0x8080000000),
                .size = UINT64_C(16) * FWK_TIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            { 0 }
        },
        .ra_mmap_table = {
            {
                .base = UINT64_C(0x00000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .remote_haid = chip2_cml0_remote_haid,
            },
            {
                .base = UINT64_C(0x8080000000),
                .size = UINT64_C(16) * FWK_TIB,
                .remote_haid = chip2_cml0_remote_haid,
            },
            { 0 }
        },
        .remote_chip_id = {
            [0] = PLATFORM_CHIP_0,
        },
        .smp_mode = true,
        .ull_to_ull_mode = true,
        .cpa_mode = true,
        .cpag_id = 0,
        .num_ports_in_cpag = 2,
    },
    // CHIP 2 --> CHIP 1
    {
        .ldid =  chip2_cml1_ldid,
        .haid =  chip2_cml1_haid,
        .remote_rnf_count = RNF_PER_CHIP_CFG2 * (PLATFORM_CHIP_COUNT - 1),
        .remote_mmap_table = {
            {
                .base = UINT64_C(0x1000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            {
                .base = UINT64_C(0x100000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            { 0 }
        },
        .ra_mmap_table = {
            {
                .base = UINT64_C(0x1000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .remote_haid = chip2_cml1_remote_haid,
            },
            {
                .base = UINT64_C(0x100000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .remote_haid = chip2_cml1_remote_haid,
            },
            { 0 }
        },
        .remote_chip_id = {
            [0] = PLATFORM_CHIP_1,
        },
        .smp_mode = true,
        .ull_to_ull_mode = true,
        .cpa_mode = true,
        .cpag_id = 1,
        .num_ports_in_cpag = 2,
    },
    // CHIP 2 --> CHIP 3
    {
        .ldid =  chip2_cml2_ldid,
        .haid =  chip2_cml2_haid,
        .remote_rnf_count = RNF_PER_CHIP_CFG2 * (PLATFORM_CHIP_COUNT - 1),
        .remote_mmap_table = {
            {
                .base = UINT64_C(0x3000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            {
                .base = UINT64_C(0x300000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            { 0 }
        },
        .ra_mmap_table = {
            {
                .base = UINT64_C(0x3000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .remote_haid = chip2_cml2_remote_haid,
            },
            {
                .base = UINT64_C(0x300000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .remote_haid = chip2_cml2_remote_haid,
            },
            { 0 }
        },
        .remote_chip_id = {
            [0] = PLATFORM_CHIP_3,
        },
        .smp_mode = true,
        .ull_to_ull_mode = true,
        .cpa_mode = true,
        .cpag_id = 2,
        .num_ports_in_cpag = 2,
    },
};
/* Chip-3 Config data */
// CHIP 3 --> CHIP 0
unsigned int chip3_cml0_ldid[] = {
    CCG_PORT_4,
    CCG_PORT_5,
};
unsigned int chip3_cml0_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_3) + CCG_PORT_4,
    (CCG_PER_CHIP * PLATFORM_CHIP_3) + CCG_PORT_5,
};
unsigned int chip3_cml0_remote_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_0) + CCG_PORT_8,
    (CCG_PER_CHIP * PLATFORM_CHIP_0) + CCG_PORT_9,
};
// CHIP 3 --> CHIP 1
unsigned int chip3_cml1_ldid[] = {
    CCG_PORT_6,
    CCG_PORT_7,
};
unsigned int chip3_cml1_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_3) + CCG_PORT_6,
    (CCG_PER_CHIP * PLATFORM_CHIP_3) + CCG_PORT_7,
};
unsigned int chip3_cml1_remote_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_1) + CCG_PORT_6,
    (CCG_PER_CHIP * PLATFORM_CHIP_1) + CCG_PORT_7,
};
// CHIP 3 --> CHIP 2
unsigned int chip3_cml2_ldid[] = {
    CCG_PORT_3,
    CCG_PORT_2,
};
unsigned int chip3_cml2_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_3) + CCG_PORT_3,
    (CCG_PER_CHIP * PLATFORM_CHIP_3) + CCG_PORT_2,
};
unsigned int chip3_cml2_remote_haid[] = {
    (CCG_PER_CHIP * PLATFORM_CHIP_2) + CCG_PORT_0,
    (CCG_PER_CHIP * PLATFORM_CHIP_2) + CCG_PORT_1,
};


static const struct mod_cmn_cyprus_ccg_config ccg_config_table_chip_3[] = {
    // CHIP 3 --> CHIP 0
    {
        .ldid =  chip3_cml0_ldid,
        .haid =  chip3_cml0_haid,
        .remote_rnf_count = RNF_PER_CHIP_CFG2 * (PLATFORM_CHIP_COUNT - 1),
        .remote_mmap_table = {
            {
                .base = UINT64_C(0x000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            {
                .base = UINT64_C(0x8080000000),
                .size = UINT64_C(16) * FWK_TIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            { 0 }
        },
        .ra_mmap_table = {
            {
                .base = UINT64_C(0x00000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .remote_haid = chip3_cml0_remote_haid,
            },
            {
                .base = UINT64_C(0x8080000000),
                .size = UINT64_C(16) * FWK_TIB,
                .remote_haid = chip3_cml0_remote_haid,
            },
            { 0 }
        },
        .remote_chip_id = {
            [0] = PLATFORM_CHIP_0,
        },
        .smp_mode = true,
        .ull_to_ull_mode = true,
        .cpa_mode = true,
        .cpag_id = 0,
        .num_ports_in_cpag = 2,
    },
    // CHIP 3 --> CHIP 1
    {
        .ldid =  chip3_cml1_ldid,
        .haid =  chip3_cml1_haid,
        .remote_rnf_count = RNF_PER_CHIP_CFG2 * (PLATFORM_CHIP_COUNT - 1),
        .remote_mmap_table = {
            {
                .base = UINT64_C(0x1000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            {
                .base = UINT64_C(0x100000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            { 0 }
        },
        .ra_mmap_table = {
            {
                .base = UINT64_C(0x1000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .remote_haid = chip3_cml1_remote_haid,
            },
            {
                .base = UINT64_C(0x100000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .remote_haid = chip3_cml1_remote_haid,
            },
            { 0 }
        },
        .remote_chip_id = {
            [0] = PLATFORM_CHIP_1,
        },
        .smp_mode = true,
        .ull_to_ull_mode = true,
        .cpa_mode = true,
        .cpag_id = 1,
        .num_ports_in_cpag = 2,
    },
    // CHIP 3 --> CHIP 2
    {
        .ldid =  chip3_cml2_ldid,
        .haid =  chip3_cml2_haid,
        .remote_rnf_count = RNF_PER_CHIP_CFG2 * (PLATFORM_CHIP_COUNT - 1),
        .remote_mmap_table = {
            {
                .base = UINT64_C(0x2000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            {
                .base = UINT64_C(0x200000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .type = MOD_CMN_CYPRUS_REGION_TYPE_CCG,
            },
            { 0 }
        },
        .ra_mmap_table = {
            {
                .base = UINT64_C(0x2000000000),
                .size = UINT64_C(64) * FWK_GIB,
                .remote_haid = chip3_cml2_remote_haid,
            },
            {
                .base = UINT64_C(0x200000000000),
                .size = UINT64_C(16) * FWK_TIB,
                .remote_haid = chip3_cml2_remote_haid,
            },
            { 0 }
        },
        .remote_chip_id = {
            [0] = PLATFORM_CHIP_2,
        },
        .smp_mode = true,
        .ull_to_ull_mode = true,
        .cpa_mode = true,
        .cpag_id = 2,
        .num_ports_in_cpag = 2,
    },
};
#endif

static struct mod_cmn_cyprus_config cmn_config_data[] = {
    [PLATFORM_CHIP_0] = {
        .base = SCP_CMN_BASE,
        .mesh_size_x = MESH_SIZE_X,
        .mesh_size_y = MESH_SIZE_Y,
        .snf_table = snf_table,
        .snf_count = FWK_ARRAY_SIZE(snf_table),
        .mmap_table = mmap,
        .mmap_count = FWK_ARRAY_SIZE(mmap),
#if (PLATFORM_VARIANT == 2)
        .ccg_config_table = ccg_config_table_chip_0,
        .ccg_table_count =
            FWK_ARRAY_SIZE(ccg_config_table_chip_0),
#endif
        .chip_addr_space = UINT64_C(64) * FWK_GIB,
        .clock_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_CLOCK,
            CFGD_MOD_CLOCK_EIDX_CMN),
        .hns_cal_mode = true,
    },
#if (PLATFORM_VARIANT == 2)
    [PLATFORM_CHIP_1] = {
        .base = SCP_CMN_BASE,
        .mesh_size_x = MESH_SIZE_X,
        .mesh_size_y = MESH_SIZE_Y,
        .snf_table = snf_table,
        .snf_count = FWK_ARRAY_SIZE(snf_table),
        .mmap_table = mmap,
        .mmap_count = FWK_ARRAY_SIZE(mmap),
        .ccg_config_table = ccg_config_table_chip_1,
        .ccg_table_count =
            FWK_ARRAY_SIZE(ccg_config_table_chip_1),
        .chip_addr_space = UINT64_C(64) * FWK_GIB,
        .clock_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_CLOCK,
            CFGD_MOD_CLOCK_EIDX_CMN),
        .hns_cal_mode = true,
    },
    [PLATFORM_CHIP_2] = {
        .base = SCP_CMN_BASE,
        .mesh_size_x = MESH_SIZE_X,
        .mesh_size_y = MESH_SIZE_Y,
        .snf_table = snf_table,
        .snf_count = FWK_ARRAY_SIZE(snf_table),
        .mmap_table = mmap,
        .mmap_count = FWK_ARRAY_SIZE(mmap),
        .ccg_config_table = ccg_config_table_chip_2,
        .ccg_table_count =
            FWK_ARRAY_SIZE(ccg_config_table_chip_2),
        .chip_addr_space = UINT64_C(64) * FWK_GIB,
        .clock_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_CLOCK,
            CFGD_MOD_CLOCK_EIDX_CMN),
        .hns_cal_mode = true,
    },
    [PLATFORM_CHIP_3] = {
        .base = SCP_CMN_BASE,
        .mesh_size_x = MESH_SIZE_X,
        .mesh_size_y = MESH_SIZE_Y,
        .snf_table = snf_table,
        .snf_count = FWK_ARRAY_SIZE(snf_table),
        .mmap_table = mmap,
        .mmap_count = FWK_ARRAY_SIZE(mmap),
        .ccg_config_table = ccg_config_table_chip_3,
        .ccg_table_count =
            FWK_ARRAY_SIZE(ccg_config_table_chip_3),
        .chip_addr_space = UINT64_C(64) * FWK_GIB,
        .clock_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_CLOCK,
            CFGD_MOD_CLOCK_EIDX_CMN),
        .hns_cal_mode = true,
    },
#endif
    [PLATFORM_CHIP_COUNT] = { 0 }
};

struct mod_cmn_cyprus_config_table cmn_driver_config = {
    .chip_config_data = &cmn_config_data[0],
    .chip_count = PLATFORM_CHIP_COUNT,
};

const struct fwk_module_config config_cmn_cyprus = {
    .data = (void *)&cmn_driver_config,
};
