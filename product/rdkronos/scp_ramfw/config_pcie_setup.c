#include "cmn_node_id.h"
#include "io_macro_layout.h"
#include "scp_cfgd_sds.h"
#include "scp_clock.h"

#include <mod_pcie_setup.h>
#include <mod_tower_nci.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>

#include <fmw_cmsis.h>

#define AP_NCI_GVP_BASE          (0x284000000UL)
#define AP_NCI_GVP_REGISTER_SIZE (0x8000000UL)
#define AP_NCI_GVP_BLOCK_BASE(idx) \
    (AP_NCI_GVP_BASE + (idx * AP_NCI_GVP_REGISTER_SIZE))
#define AP_NCI_GVP_MAPPED_BASE (0x80000000UL)
#define AP_NCI_GVP_MAPPED_SIZE (16 * FWK_MIB)
#define MAPPED_ECAM_BASE       (0x88000000)

#define CHIP_ADDRESS_SPACE_SIZE        (UINT64_C(64) * FWK_GIB)
#define CHIP_ADDRESS_SPACE_OFFSET(idx) (idx * CHIP_ADDRESS_SPACE_SIZE)
#define CHIP_ECAM_BASE_ADDRESS(idx) \
    (0x4000000000ULL + CHIP_ADDRESS_SPACE_OFFSET(idx))
#define CHIP_MMIOL_BASE_ADDRESS(idx) \
    (0x4040000000ULL + CHIP_ADDRESS_SPACE_OFFSET(idx))
#define CHIP0_PCIE_MMIOL_BASE (0x60000000UL)
/* PCIe ECAM and MMIO addresses */
#define CHIP_PCIE_ECAM_SIZE  (0x10000000UL)
#define CHIP_PCIE_MMIOH_SIZE (0x4000000000ULL)
#define CHIP_PCIE_BUS_SIZE   256
#define CHIP_PCIE_MMIOL_SIZE (0x8000000UL)

#define IO_MACRO_PCIE_ELEMENT_CONFIG(idx, x1_en, x20_en, x21_en, x4_en, x8_en) \
    { \
        .name = "IO Macro " #idx, \
        .data = &((struct mod_pcie_setup_config){ \
            .type = MOD_PCIE_SETUP_DEV_PCIE, \
            .x1 = { .valid = x1_en, .allow_ns_access = true }, \
            .x2_0 = { .valid = x20_en, .allow_ns_access = true }, \
            .x2_1 = { .valid = x21_en, .allow_ns_access = true }, \
            .x4 = { .valid = x4_en, .allow_ns_access = true }, \
            .x8 = { .valid = x8_en, .allow_ns_access = true }, \
            .cmn_node_id = IOVB_NODE_ID##idx, \
            .reg_base = AP_NCI_GVP_BLOCK_BASE(idx), \
            .clock_id = FWK_ID_ELEMENT_INIT( \
                FWK_MODULE_IDX_CLOCK, CFGD_MOD_CLOCK_EIDX_CMN), \
            .sds_struct_id = SDS_PCIE_MMAP_INFO, \
            .hostbridge_id = idx, \
            .reg_map = ((struct tower_nci_psam_region[]){ \
                { \
                    .node_id = AMNI_PMNI_TCU_APB, \
                    .base_address = \
                        0x280000000 + (idx * AP_NCI_GVP_REGISTER_SIZE), \
                    .size = 0x4000000, \
                }, \
                { \
                    .node_id = AMNI_PMNI_CTRL_REG_APB, \
                    .base_address = \
                        0x285B00000 + (idx * AP_NCI_GVP_REGISTER_SIZE), \
                    .size = 0x10000, \
                }, \
                { 0 } }), \
        }) \
    }

#define CHIP_MEMMAP(idx) \
    { \
        .chip_address_space = CHIP_ADDRESS_SPACE_SIZE, \
        .mmap = { \
            .ecam1 = { \
                .start = CHIP_ECAM_BASE_ADDRESS(idx), \
                .size = CHIP_PCIE_ECAM_SIZE, \
            }, \
            .mmiol = { \
                .start = CHIP##idx##_PCIE_MMIOL_BASE, \
                .size = CHIP_PCIE_MMIOL_SIZE, \
            }, \
            .mmioh = { \
                .start = CHIP_MMIOL_BASE_ADDRESS(idx), \
                .size = CHIP_PCIE_MMIOH_SIZE \
            }, \
            .bus = { \
                .start = 0, \
                .size = CHIP_PCIE_BUS_SIZE \
            } \
        }, \
        .x1_base_interrupt_id = 0x0, \
        .x2_0_base_interrupt_id = 0x10000, \
        .x2_1_base_interrupt_id = 0x10000, \
        .x4_base_interrupt_id = 0x30000, \
        .x8_base_interrupt_id = 0x40000, \
        .mapped_ecam_base = MAPPED_ECAM_BASE, \
        .mapped_nci_gvp_base = AP_NCI_GVP_MAPPED_BASE, \
        .mapped_nci_gvp_size = AP_NCI_GVP_MAPPED_SIZE, \
        .x1_amni_id = AMNI_PCIEX1_0, \
        .x2_0_amni_id = AMNI_PCIEX2_0, \
        .x2_1_amni_id = AMNI_PCIEX2_1, \
        .x4_amni_id = AMNI_PCIEX4_0, \
        .x8_amni_id = AMNI_PCIEX8_0, \
        .asni_id = ASNI_CMN, \
    }

static const struct fwk_element pcie_setup_element_table[] = {
    IO_MACRO_PCIE_ELEMENT_CONFIG(0, false, false, false, false, true),
    IO_MACRO_PCIE_ELEMENT_CONFIG(1, false, false, false, false, true),
    IO_MACRO_PCIE_ELEMENT_CONFIG(2, false, false, false, false, true),
    IO_MACRO_PCIE_ELEMENT_CONFIG(3, false, false, false, false, true),
    { 0 }
};

static const struct mod_pcie_setup_resource_info resource_info[] = {
    CHIP_MEMMAP(0),
    { 0 }
};

static const struct fwk_element *pcie_setup_get_element_table(
    fwk_id_t module_id)
{
    return pcie_setup_element_table;
}

struct fwk_module_config config_pcie_setup = {
    .data = resource_info,
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(pcie_setup_get_element_table),
};
