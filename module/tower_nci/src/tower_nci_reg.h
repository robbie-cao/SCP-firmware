/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef TOWER_NCI_REG_H
#define TOWER_NCI_REG_H

#include <stdint.h>

#define TOWER_NCI_MAX_NUM_REGIONS 128
#define TOWER_NCI_MAX_NUM_HTG_REGIONS 32
#define TOWER_NCI_MAX_NUM_HTG_TGID_NUM 32
#define TOWER_NCI_MAX_NUM_TOP_ADDR_CFG 32

#define PSAM_REGION_SELECT(register, region) \
        *((&(register)) + ((0x10 * (region)) >> 2))

/* Tower NCI Domain top registers. These are common for all domains */
struct tower_nci_domain_cfg_hdr {
    union {
        volatile uint32_t node_type;
        struct {
            volatile uint32_t type:16;
            volatile uint32_t id:16;
        };
    };
    volatile uint32_t child_node_info;
    volatile uint32_t x_pointers[];
};

/* Tower NCI Sub-feature register set. Found in component domain */
struct tower_nci_sub_feature_cfg_attr {
    union {
        volatile uint32_t node_type;
        struct {
            volatile uint32_t type:16;
            volatile uint32_t reserved:16;
        };
    };
    volatile uint32_t pointer;
};

/* Tower NCI Component top registers. These are common for all components */
struct tower_nci_component_cfg_hdr {
    volatile uint32_t node_type;
             uint32_t reserved_0[4];
    volatile uint32_t interface_id_0_3;
             uint32_t reserved_1[58];
    volatile uint32_t num_sub_features;
             uint32_t reserved_2;
    struct tower_nci_sub_feature_cfg_attr
                      sub_feature[];
};

struct nh_region_regs {
    union {
        volatile uint64_t cfg1_cfg0;
        struct {
            volatile uint32_t cfg0;
            volatile uint32_t cfg1;
        };
    };
    union {
        volatile uint64_t cfg3_cfg2;
        struct {
            volatile uint32_t cfg2;
            volatile uint32_t cfg3;
        };
    };
};

struct htg_region_regs {
    union {
        volatile uint64_t cfg1_cfg0;
        struct {
            volatile uint32_t cfg0;
            volatile uint32_t cfg1;
        };
    };
    union {
        volatile uint64_t cfg3_cfg2;
        struct {
            volatile uint32_t cfg2;
            volatile uint32_t cfg3;
        };
    };
};

/**
 * \brief Tower NCI PSAM register map
 */
struct tower_nci_psam_reg_map {
    volatile uint32_t sam_unit_info;
    volatile uint32_t sam_status;
             uint32_t reserved__0[2];
    volatile uint32_t htg_addr_mask_l;
    volatile uint32_t htg_addr_mask_u;
    volatile uint32_t axid_mask;
             uint32_t reserved_0;
    volatile uint32_t cmp_addr_mask_l;
    volatile uint32_t cmp_addr_mask_u;
             uint32_t reserved_1[2];
    volatile uint32_t generic_config_reg0;
    volatile uint32_t generic_config_reg1;
             uint32_t reserved_2[50];
    volatile struct nh_region_regs nh_region[TOWER_NCI_MAX_NUM_REGIONS];
    volatile struct htg_region_regs htg_region[TOWER_NCI_MAX_NUM_HTG_REGIONS];
    volatile uint32_t htg_tgtid_cfg[TOWER_NCI_MAX_NUM_HTG_TGID_NUM];
    volatile uint32_t np2_top_addr_cfg[TOWER_NCI_MAX_NUM_TOP_ADDR_CFG];
};

#endif /* TOWER_NCI_REG_H */
