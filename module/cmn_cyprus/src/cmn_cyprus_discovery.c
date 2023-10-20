/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions and utility functions for CMN-Cyprus Discovery.
 */

#include <internal/cmn_cyprus_ctx.h>
#include <internal/cmn_cyprus_discovery.h>
#include <internal/cmn_cyprus_reg.h>
#include <internal/cmn_cyprus_utils.h>

#include <mod_cmn_cyprus.h>

#include <fwk_assert.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_status.h>

#include <inttypes.h>
#include <stdint.h>

/* Max Node Counts */
#define MAX_HNS_COUNT 128
#define MAX_RND_COUNT 40
#define MAX_RNI_COUNT 40
#define MAX_RNF_COUNT 256

/* Peripheral ID Revision Numbers */
#define CMN_CYPRUS_PERIPH_ID_2_REV_R0_P0 (0x00)
#define CMN_CYPRUS_PERIPH_ID_2_REV_R1_P0 (0x01)
#define CMN_CYPRUS_PERIPH_ID_2_REV_R2_P0 (0x02)
#define CMN_CYPRUS_PERIPH_ID_2_REV_R3_P0 (0x03)
#define CMN_CYPRUS_PERIPH_ID_UNKNOWN_REV (CMN_CYPRUS_PERIPH_ID_2_REV_R3_P0 + 1)

/* Peripheral ID Revision Numbers */
#define CMN_CYPRUS_PERIPH_ID_2_MASK    UINT64_C(0xFF)
#define CMN_CYPRUS_PERIPH_ID_2_REV_POS 4

/* Node Info */
#define CMN_CYPRUS_NODE_INFO_TYPE      UINT64_C(0x000000000000FFFF)
#define CMN_CYPRUS_NODE_ID_DEVICE_MASK (0x3)
#define CMN_CYPRUS_NODE_ID_PORT_POS    2
#define CMN_CYPRUS_NODE_ID_PORT_MASK   (0x1)
#define CMN_CYPRUS_NODE_ID_Y_POS       3

/* Child Info */
#define CMN_CYPRUS_CHILD_INFO_COUNT     UINT64_C(0x000000000000FFFF)
#define CMN_CYPRUS_CHILD_POINTER_OFFSET UINT64_C(0x000000003FFFFFFF)
#define CMN_CYPRUS_CHILD_POINTER_EXT    UINT64_C(0x0000000080000000)

/* External child node */
#define CMN_CYPRUS_CHILD_POINTER_EXT_REGISTER_OFFSET  UINT64_C(0x00003FFF)
#define CMN_CYPRUS_CHILD_POINTER_EXT_NODE_POINTER     UINT64_C(0x3FFF0000)
#define CMN_CYPRUS_CHILD_POINTER_EXT_NODE_POINTER_POS 16

/* MXP device port */
#define CMN_CYPRUS_MXP_NODE_INFO_NUM_DEVICE_PORT_MASK UINT64_C(0xF000000000000)
#define CMN_CYPRUS_MXP_NODE_INFO_NUM_DEVICE_PORT_POS  48

/* For MXP with 3 or 4 ports */
#define CMN_CYPRUS_MULTI_PORTS_NODE_ID_PORT_POS  1
#define CMN_CYPRUS_MULTI_PORTS_NODE_ID_PORT_MASK (0x3)

/* MXP Port Connect Info */
#define CMN_CYPRUS_MXP_PORT_CONNECT_INFO_DEVICE_TYPE_MASK   UINT64_C(0x3F)
#define CMN_CYPRUS_MXP_PORT_CONNECT_INFO_CAL_CONNECTED_MASK UINT64_C(0x80)
#define CMN_CYPRUS_MXP_PORT_CONNECT_INFO_CAL_CONNECTED_POS  7

#define CMN_CYPRUS_MXP_PORT_DISABLE_PORT_OFFSET UINT64_C(0x4)

enum node_type {
    NODE_TYPE_INVALID = 0x0,
    NODE_TYPE_DVM = 0x1,
    NODE_TYPE_CFG = 0x2,
    NODE_TYPE_DTC = 0x3,
    NODE_TYPE_HN_I = 0x4,
    NODE_TYPE_XP = 0x6,
    NODE_TYPE_SBSX = 0x7,
    NODE_TYPE_RN_I = 0xA,
    NODE_TYPE_RN_D = 0xD,
    NODE_TYPE_RN_SAM = 0xF,
    NODE_TYPE_HN_P = 0x11,
    NODE_TYPE_CXRA = 0x100,
    NODE_TYPE_CXHA = 0x101,
    NODE_TYPE_CXLA = 0x102,
    NODE_TYPE_CCRA = 0x103,
    NODE_TYPE_CCHA = 0x104,
    NODE_TYPE_CCLA = 0x105,
    NODE_TYPE_HN_S = 0x200,
    NODE_TYPE_MPAM_S = 0x201,
    NODE_TYPE_MPAM_NS = 0x202,
};

enum device_type {
    DEVICE_TYPE_RN_F_CHIB_ESAM = 0x5, // 0b00101
    DEVICE_TYPE_HN_F = 0xF, // 0b01111
    DEVICE_TYPE_CXHA = 0x11, // 0b10001
    DEVICE_TYPE_CXRA = 0x12, // 0b10010
    DEVICE_TYPE_CXRH = 0x13, // 0b10011
    DEVICE_TYPE_HN_S = 0x1A, // 0b11010
    DEVICE_TYPE_CCG = 0x1E, // 0b11110
    DEVICE_TYPE_RN_F_CHID_ESAM = 0x15, // 0b10101
    DEVICE_TYPE_RN_F_CHIC_ESAM = 0x17, // 0b10111
    DEVICE_TYPE_RN_F_CHIE_ESAM = 0x19, // 0b11001
    DEVICE_TYPE_RN_F_CHIF_ESAM = 0x21, // 0b100001
};

/*
 * Encoding bits size of the X and Y position in the Node info value.
 * If X and Y dimension are less than 4, encoding bits size will be 2.
 * If X and Y dimension are between 5 and 8, encoding bits size will be 3.
 */
static unsigned int encoding_bits;
/* Mask to get the x and y coordinates from node position */
static unsigned int mask_bits;

/* CMN Cyprus revision names */
static const char *const cmn_cyprus_rev_to_name[] = {
    [CMN_CYPRUS_PERIPH_ID_2_REV_R0_P0] = "r0p0",
    [CMN_CYPRUS_PERIPH_ID_2_REV_R1_P0] = "r1p0",
    [CMN_CYPRUS_PERIPH_ID_2_REV_R2_P0] = "r2p0",
    [CMN_CYPRUS_PERIPH_ID_2_REV_R3_P0] = "r3p0",
    [CMN_CYPRUS_PERIPH_ID_UNKNOWN_REV] = "Unknown!",
};

static void set_encoding_and_masking_bits(struct cmn_cyprus_ctx *ctx)
{
    unsigned int mesh_size_x;
    unsigned int mesh_size_y;

    mesh_size_x = ctx->config->mesh_size_x;
    mesh_size_y = ctx->config->mesh_size_y;

    /*
     * Determine the number of bits used to represent each node coordinate based
     * on the mesh size as per CMN Cyprus specification.
     */
    if ((mesh_size_x > 8) || (mesh_size_y > 8)) {
        encoding_bits = 4;
    } else {
        encoding_bits = ((mesh_size_x > 4) || (mesh_size_y > 4)) ? 3 : 2;
    }

    mask_bits = (1 << encoding_bits) - 1;
}

static unsigned int get_node_pos_x(unsigned int node_id)
{
    return (node_id >> (CMN_CYPRUS_NODE_ID_Y_POS + encoding_bits)) & mask_bits;
}

static unsigned int get_node_pos_y(unsigned int node_id)
{
    return (node_id >> CMN_CYPRUS_NODE_ID_Y_POS) & mask_bits;
}

static unsigned int get_cmn_cyprus_revision(struct cmn_cyprus_cfgm_reg *root)
{
    return (root->PERIPH_ID[1] & CMN_CYPRUS_PERIPH_ID_2_MASK) >>
        CMN_CYPRUS_PERIPH_ID_2_REV_POS;
}

static const char *get_cmn_cyprus_revision_name(
    struct cmn_cyprus_cfgm_reg *root)
{
    unsigned int revision;

    /* Read the revision from the configuration register */
    revision = get_cmn_cyprus_revision(root);

    if (revision > CMN_CYPRUS_PERIPH_ID_UNKNOWN_REV) {
        revision = CMN_CYPRUS_PERIPH_ID_UNKNOWN_REV;
    }

    return cmn_cyprus_rev_to_name[revision];
}

static enum node_type get_node_type(void *node_base)
{
    struct node_header *node = node_base;
    return (enum node_type)(node->NODE_INFO & CMN_CYPRUS_NODE_INFO_TYPE);
}

static unsigned int get_node_child_count(void *node_base)
{
    struct node_header *node = node_base;
    return node->CHILD_INFO & CMN_CYPRUS_CHILD_INFO_COUNT;
}

static void *get_child_node(
    uintptr_t base,
    void *node_base,
    unsigned int child_index)
{
    struct node_header *node = node_base;
    uint32_t child_pointer;
    unsigned int offset;
    void *child_node;

    child_pointer = node->CHILD_POINTER[child_index];
    offset = child_pointer & CMN_CYPRUS_CHILD_POINTER_OFFSET;

    child_node = (void *)(base + offset);
    return child_node;
}

const char *const type_to_name[] = {
    [NODE_TYPE_INVALID] = "<Invalid>",
    [NODE_TYPE_DVM] = "DVM",
    [NODE_TYPE_CFG] = "CFG",
    [NODE_TYPE_DTC] = "DTC",
    [NODE_TYPE_HN_I] = "HN-I",
    [NODE_TYPE_HN_S] = "HN-S",
    [NODE_TYPE_XP] = "XP",
    [NODE_TYPE_SBSX] = "SBSX",
    [NODE_TYPE_MPAM_S] = "MPAM-S",
    [NODE_TYPE_MPAM_NS] = "MPAM-NS",
    [NODE_TYPE_RN_I] = "RN-I",
    [NODE_TYPE_RN_D] = "RN-D",
    [NODE_TYPE_RN_SAM] = "RN-SAM",
    [NODE_TYPE_HN_P] = "HN-P",
    [NODE_TYPE_CXRA] = "CXRA",
    [NODE_TYPE_CXHA] = "CXHA",
    [NODE_TYPE_CXLA] = "CXLA",
    [NODE_TYPE_CCRA] = "CCRA",
    [NODE_TYPE_CCHA] = "CCHA",
    [NODE_TYPE_CCLA] = "CCLA",
};

static const char *get_node_type_name(enum node_type node_type)
{
    /* NODE_TYPE_MPAM_NS has the highest node type value i.e. 0x202 */
    if (node_type <= NODE_TYPE_MPAM_NS) {
        return type_to_name[node_type];
    }

    /* Invalid node IDs */
    return type_to_name[NODE_TYPE_INVALID];
}

static bool is_child_external(void *node_base, unsigned int child_index)
{
    struct node_header *node = node_base;

    /* Read External Child Node indicator, bit[31] */
    return !!(node->CHILD_POINTER[child_index] & (1U << 31));
}

static unsigned int get_child_node_id(void *node_base, unsigned int child_index)
{
    struct node_header *node = node_base;
    uint32_t node_pointer;
    unsigned int node_id;

    node_pointer = (node->CHILD_POINTER[child_index] &
                    CMN_CYPRUS_CHILD_POINTER_EXT_NODE_POINTER) >>
        CMN_CYPRUS_CHILD_POINTER_EXT_NODE_POINTER_POS;

    /*
     * For mesh widths using 2 bits each for X,Y encoding:
     * NodeID[1:0] = DeviceID[3:2]
     * NodeID[2]   = DeviceID[0]
     * NodeID[4:3] = NODE POINTER[7:6]
     * NodeID[6:5] = NODE POINTER[9:8]
     *
     * For mesh widths using 3 bits each for X,Y encoding:
     * NodeID[1:0] = DeviceID[3:2]
     * NodeID[2]   = DeviceID[0]
     * NodeID[5:3] = NODE POINTER[8:6]
     * NodeID[8:6] = NODE POINTER[11:9]
     */
    node_id = (((node_pointer >> 6) & 0xff) << 3) |
        ((node_pointer & 0x1) << 2) | ((node_pointer >> 2) & 0x3);

    return node_id;
}

static unsigned int get_node_device_port_count(void *node_base)
{
    struct node_header *node = node_base;
    return (node->NODE_INFO & CMN_CYPRUS_MXP_NODE_INFO_NUM_DEVICE_PORT_MASK) >>
        CMN_CYPRUS_MXP_NODE_INFO_NUM_DEVICE_PORT_POS;
}

static unsigned int get_port_number(
    unsigned int child_node_id,
    unsigned int xp_port_cnt)
{
    if (xp_port_cnt <= 2) {
        return (child_node_id >> CMN_CYPRUS_NODE_ID_PORT_POS) &
            CMN_CYPRUS_NODE_ID_PORT_MASK;
    } else {
        /* For port counts 3 and 4 */
        return (child_node_id >> CMN_CYPRUS_MULTI_PORTS_NODE_ID_PORT_POS) &
            CMN_CYPRUS_MULTI_PORTS_NODE_ID_PORT_MASK;
    }
}

static unsigned int get_device_type(void *mxp_base, int port)
{
    struct cmn_cyprus_mxp_reg *mxp = mxp_base;
    return mxp->PORT_CONNECT_INFO[port] &
        CMN_CYPRUS_MXP_PORT_CONNECT_INFO_DEVICE_TYPE_MASK;
}

static bool is_device_type_rnf(void *mxp_base, bool port)
{
    return (
        (get_device_type(mxp_base, port) == DEVICE_TYPE_RN_F_CHIB_ESAM) ||
        (get_device_type(mxp_base, port) == DEVICE_TYPE_RN_F_CHIC_ESAM) ||
        (get_device_type(mxp_base, port) == DEVICE_TYPE_RN_F_CHID_ESAM) ||
        (get_device_type(mxp_base, port) == DEVICE_TYPE_RN_F_CHIE_ESAM) ||
        (get_device_type(mxp_base, port) == DEVICE_TYPE_RN_F_CHIF_ESAM));
}

static bool is_cal_connected(void *mxp_base, bool port)
{
    struct cmn_cyprus_mxp_reg *mxp = mxp_base;
    return (mxp->PORT_CONNECT_INFO[port] &
            CMN_CYPRUS_MXP_PORT_CONNECT_INFO_CAL_CONNECTED_MASK) >>
        CMN_CYPRUS_MXP_PORT_CONNECT_INFO_CAL_CONNECTED_POS;
}

static unsigned int get_device_number(unsigned int node_id)
{
    return (node_id & CMN_CYPRUS_NODE_ID_DEVICE_MASK);
}

static void enable_mxp_device(
    void *mxp_base,
    unsigned int port,
    unsigned int device)
{
    struct cmn_cyprus_mxp_reg *mxp = (struct cmn_cyprus_mxp_reg *)mxp_base;
    uint8_t port_offset, device_mask;

    port_offset = (port * CMN_CYPRUS_MXP_PORT_DISABLE_PORT_OFFSET);
    device_mask = (0x1 << device);

    /* Enable device in the XP port */
    mxp->PORT_DISABLE &= ~(device_mask << port_offset);
}

static bool is_node_pos_equal(struct node_pos *node1, struct node_pos *node2)
{
    /* Check if X and Y positions match */
    if ((node1->pos_x == node2->pos_x) && (node1->pos_y == node2->pos_y)) {
        /* Check if port number and device number match */
        if ((node1->port_num == node2->port_num) &&
            (node1->device_num == node2->device_num)) {
            return true;
        }
    }

    return false;
}

static bool is_hns_isolated(
    struct node_pos *hns_pos,
    struct cmn_cyprus_ctx *ctx)
{
    unsigned int idx;
    struct isolated_hns_node_info *isolated_hns_table;

    isolated_hns_table = ctx->config->isolated_hns_table;

    for (idx = 0; idx < ctx->config->isolated_hns_count; idx++) {
        if (is_node_pos_equal(hns_pos, &isolated_hns_table[idx].hns_pos)) {
            FWK_LOG_ERR(
                MOD_NAME "  P%u, D%u, Isolated HN-S",
                hns_pos->port_num,
                hns_pos->device_num);
            return true;
        }
    }

    return false;
}

/*
 * Disable MXP device isolation for non-isolated HN-S nodes.
 */
static void disable_hns_isolation(
    struct cmn_cyprus_mxp_reg *xp,
    struct cmn_cyprus_ctx *ctx)
{
    unsigned int port_num, device_num;
    unsigned int device_type;
    struct node_pos hns_pos;

    /* Iterate through each port in the XP */
    for (port_num = 0; port_num < 6; port_num++) {
        device_type = get_device_type(xp, port_num);

        if ((device_type != DEVICE_TYPE_HN_S) &&
            (device_type != DEVICE_TYPE_HN_F)) {
            continue;
        }

        /* Iterate through each device in the XP port */
        for (device_num = 0; device_num < 4; device_num++) {
            /* Initialize node position of the HN-S node */
            hns_pos.pos_x = get_node_pos_x(get_node_id((void *)xp));
            hns_pos.pos_y = get_node_pos_y(get_node_id((void *)xp));
            hns_pos.port_num = port_num;
            hns_pos.device_num = device_num;

            if (is_hns_isolated(&hns_pos, ctx)) {
                /* Skip HN-S nodes that ought to be isolated */
                continue;
            }

            /* Disable HN-S isolation */
            enable_mxp_device(xp, port_num, device_num);
        }
    }
}

static bool is_node_isolated(uintptr_t node_base, struct cmn_cyprus_ctx *ctx)
{
    unsigned int idx;
    struct isolated_hns_node_info *isolated_hns_table;

    isolated_hns_table = ctx->config->isolated_hns_table;

    /* Iterate through the list of HN-S nodes to be isolated */
    for (idx = 0; idx < ctx->config->isolated_hns_count; idx++) {
        if (node_base == isolated_hns_table[idx].hns_base) {
            return true;
        }
    }

    return false;
}

/*
 * Discover the topology of the interconnect.
 */
static int discover_mesh_topology(struct cmn_cyprus_ctx *ctx)
{
    unsigned int ccg_ra_reg_count;
    unsigned int ccg_ha_reg_count;
    unsigned int ccla_reg_count;
    unsigned int cxg_ra_reg_count;
    unsigned int cxg_ha_reg_count;
    unsigned int cxla_reg_count;
    unsigned int node_count;
    unsigned int node_idx;
    unsigned int xp_count;
    unsigned int xp_idx;
    unsigned int xp_port;
    const struct mod_cmn_cyprus_config *config;
    struct node_header *node;
    struct cmn_cyprus_mxp_reg *xp;

    ccg_ra_reg_count = 0;
    ccg_ha_reg_count = 0;
    ccla_reg_count = 0;
    cxg_ra_reg_count = 0;
    cxg_ha_reg_count = 0;
    cxla_reg_count = 0;

    config = ctx->config;

    /* Configure the encoding and masking bits based on the mesh size */
    set_encoding_and_masking_bits(ctx);

    FWK_LOG_INFO(
        MOD_NAME "CMN-CYPRUS revision: %s",
        get_cmn_cyprus_revision_name(ctx->root));
    FWK_LOG_INFO(MOD_NAME "Starting discovery...");
    FWK_LOG_DEBUG(
        MOD_NAME "Rootnode Base address: 0x%" PRIxPTR, (uintptr_t)ctx->root);

    fwk_assert(get_node_type(ctx->root) == NODE_TYPE_CFG);

    /* Get number of cross points in the mesh */
    xp_count = get_node_child_count(ctx->root);

    /* Traverse cross points (XP) */
    for (xp_idx = 0; xp_idx < xp_count; xp_idx++) {
        /* Pointer to the cross point register */
        xp = get_child_node(config->base, ctx->root, xp_idx);

        fwk_assert(get_node_type(xp) == NODE_TYPE_XP);

        FWK_LOG_INFO(
            MOD_NAME "XP (%d, %d) ID:%d, LID:%d",
            get_node_pos_x(get_node_id(xp)),
            get_node_pos_y(get_node_id(xp)),
            get_node_id(xp),
            get_node_logical_id(xp));

        disable_hns_isolation(xp, ctx);

        /* Get number of child nodes connected to the cross point */
        node_count = get_node_child_count(xp);

        /* Traverse nodes */
        for (node_idx = 0; node_idx < node_count; node_idx++) {
            /* Pointer to the child node header */
            node = get_child_node(config->base, xp, node_idx);

            /* Check if the node must be skipped due to HN-S isolation */
            if (is_node_isolated((uintptr_t)node, ctx)) {
                continue;
            }

            /*
             * Get the port number in the cross point to which the node is
             * connected to.
             */
            xp_port = get_port_number(
                get_node_id(node), get_node_device_port_count(xp));

            FWK_LOG_INFO(
                MOD_NAME "  P%d, %s ID:%d, LID:%d",
                xp_port,
                get_node_type_name(get_node_type(node)),
                get_node_id(node),
                get_node_logical_id(node));

            if (is_child_external(xp, node_idx)) { /* External nodes */
                /*
                 * Get the port number in the cross point to which the child
                 * node is connected to.
                 */
                xp_port = get_port_number(
                    get_child_node_id(xp, node_idx),
                    get_node_device_port_count(xp));

                /*
                 * If the device type is CXRH, CXHA, or CXRA, then the external
                 * child node is CXLA as every CXRH, CXHA, or CXRA node has a
                 * corresponding external CXLA node.
                 */
                if ((get_device_type(xp, xp_port) == DEVICE_TYPE_CXRH) ||
                    (get_device_type(xp, xp_port) == DEVICE_TYPE_CXHA) ||
                    (get_device_type(xp, xp_port) == DEVICE_TYPE_CXRA)) {
                    cxla_reg_count++;
                    FWK_LOG_INFO(
                        MOD_NAME "  Found CXLA at node ID: %d",
                        get_child_node_id(xp, node_idx));
                }
            } else { /* Internal nodes */
                switch (get_node_type(node)) {
                case NODE_TYPE_HN_S:
                    if (ctx->hns_count >= MAX_HNS_COUNT) {
                        FWK_LOG_INFO(
                            MOD_NAME "  hns count %d >= max limit (%d)",
                            ctx->hns_count,
                            MAX_HNS_COUNT);
                        return FWK_E_DATA;
                    }
                    ctx->hns_count++;
                    break;

                case NODE_TYPE_RN_SAM:
                    ctx->rnsam_count++;

                    /*
                     * RN-F nodes don't have node type identifier and hence
                     * the count cannot be determined using the node type id.
                     * Alternatively, check if the device type connected to the
                     * Crosspoint (XP) is one of the RNF types and determine the
                     * RN-F count (if CAL connected RN-F, double the count).
                     */
                    xp_port = get_port_number(
                        get_node_id(node), get_node_device_port_count(xp));

                    if (is_device_type_rnf(xp, xp_port)) {
                        if (is_cal_connected(xp, xp_port)) {
                            ctx->rnf_count += 2;
                            FWK_LOG_INFO(
                                MOD_NAME
                                "  RN-F (CAL connected) found at port: %d",
                                xp_port);
                        } else {
                            ctx->rnf_count++;
                            FWK_LOG_INFO(
                                MOD_NAME "  RN-F found at port: %d", xp_port);
                        }
                    }
                    break;

                case NODE_TYPE_RN_D:
                    if ((ctx->rnd_count) >= MAX_RND_COUNT) {
                        FWK_LOG_ERR(
                            MOD_NAME "  rnd count %d >= max limit (%d)\n",
                            ctx->rnd_count,
                            MAX_RND_COUNT);
                        return FWK_E_DATA;
                    }
                    ctx->rnd_count++;
                    break;

                case NODE_TYPE_RN_I:
                    if ((ctx->rni_count) >= MAX_RNI_COUNT) {
                        FWK_LOG_ERR(
                            MOD_NAME "  rni count %d >= max limit (%d)\n",
                            ctx->rni_count,
                            MAX_RNI_COUNT);
                        return FWK_E_DATA;
                    }
                    ctx->rni_count++;
                    break;

                case NODE_TYPE_CCRA:
                    ccg_ra_reg_count++;
                    break;

                case NODE_TYPE_CCHA:
                    ccg_ha_reg_count++;
                    break;

                case NODE_TYPE_CCLA:
                    ccla_reg_count++;
                    break;

                case NODE_TYPE_CXRA:
                    cxg_ra_reg_count++;
                    break;

                case NODE_TYPE_CXHA:
                    cxg_ha_reg_count++;
                    break;

                /* CXLA should not be an internal node */
                case NODE_TYPE_CXLA:
                    FWK_LOG_ERR(MOD_NAME
                                "CXLA node should not be internal node, "
                                "discovery failed");
                    return FWK_E_DEVICE;
                    break;

                default:
                    /* Nothing to be done for other node types */
                    break;
                }
            }
        }
    }

    FWK_LOG_INFO(MOD_NAME "Total RN-SAM nodes: %d", ctx->rnsam_count);
    FWK_LOG_INFO(
        MOD_NAME "Total HN-S nodes: %d",
        (ctx->hns_count + ctx->config->isolated_hns_count));
    FWK_LOG_INFO(
        MOD_NAME "Isolated HN-S nodes: %d", ctx->config->isolated_hns_count);
    FWK_LOG_INFO(MOD_NAME "Total RN-D nodes: %d", ctx->rnd_count);
    FWK_LOG_INFO(MOD_NAME "Total RN-F nodes: %d", ctx->rnf_count);
    FWK_LOG_INFO(MOD_NAME "Total RN-I nodes: %d", ctx->rni_count);
    FWK_LOG_INFO(
        MOD_NAME "Total CCIX Request Agent nodes: %d", cxg_ra_reg_count);
    FWK_LOG_INFO(MOD_NAME "Total CCIX Home Agent nodes: %d", cxg_ha_reg_count);
    FWK_LOG_INFO(MOD_NAME "Total CCIX Link Agent nodes: %d", cxla_reg_count);
    FWK_LOG_INFO(
        MOD_NAME "Total CCG Request Agent nodes: %d", ccg_ra_reg_count);
    FWK_LOG_INFO(MOD_NAME "Total CCG Home Agent nodes: %d", ccg_ha_reg_count);
    FWK_LOG_INFO(MOD_NAME "Total CCG Link Agent nodes: %d", ccla_reg_count);

    ctx->ccg_node_count = ccg_ra_reg_count;

    /* When CAL is present, the number of HN-S nodes must be even */
    if ((ctx->hns_count % 2 != 0) && (config->hns_cal_mode == true)) {
        FWK_LOG_ERR(
            MOD_NAME "Error! hns count: %d should be even when CAL mode is set",
            ctx->hns_count);
        return FWK_E_DATA;
    }

    /*
     * Check if the total number of HN-S nodes is a power of two.
     *
     * Note: This check is required as the driver currently supports
     * power of two hashing or hierarchical hashing only.
     */
    if ((__builtin_popcount(ctx->hns_count)) != 1) {
        FWK_LOG_ERR(MOD_NAME "Error! Total HN-S count is not a power of two");
        return FWK_E_DATA;
    }

    /* Include the isolated HNS-nodes in the total node count */
    ctx->hns_count = (ctx->hns_count + ctx->config->isolated_hns_count);

    if (ctx->rnf_count > MAX_RNF_COUNT) {
        FWK_LOG_ERR(
            MOD_NAME "Error! rnf count %d > max limit (%d)\n",
            ctx->rnf_count,
            MAX_RNF_COUNT);
        return FWK_E_RANGE;
    }

    return FWK_SUCCESS;
}

/* Traverse the mesh and initialize the module context data */
static void cmn_cyprus_init_ctx(struct cmn_cyprus_ctx *ctx)
{
    unsigned int ldid;
    unsigned int node_count;
    unsigned int node_id;
    unsigned int node_idx;
    unsigned int xp_count;
    unsigned int xp_idx;
    unsigned int rnsam_entry;
    unsigned int xp_port;
    void *node;
    struct cmn_cyprus_mxp_reg *xp;
    const struct mod_cmn_cyprus_config *config = ctx->config;

    fwk_assert(get_node_type(ctx->root) == NODE_TYPE_CFG);

    rnsam_entry = 0;

    /* Get the number of cross points in the mesh */
    xp_count = get_node_child_count(ctx->root);

    /* Traverse cross points (XP) */
    for (xp_idx = 0; xp_idx < xp_count; xp_idx++) {
        /* Pointer to the cross point register */
        xp = get_child_node(config->base, ctx->root, xp_idx);

        fwk_assert(get_node_type(xp) == NODE_TYPE_XP);

        /* Get the number of child nodes connected to the cross point */
        node_count = get_node_child_count(xp);

        /* Traverse nodes */
        for (node_idx = 0; node_idx < node_count; node_idx++) {
            /* Pointer to child node */
            node = get_child_node(config->base, xp, node_idx);

            /* Check if the node must be skipped due to HN-S isolation */
            if (is_node_isolated((uintptr_t)node, ctx)) {
                continue;
            }

            /* Get node id */
            node_id = get_node_id(node);

            /*
             * Get the port number in the cross point to which the child
             * node is connected to.
             */
            xp_port = get_port_number(
                get_child_node_id(xp, node_idx),
                get_node_device_port_count(xp));

            if (!is_child_external(xp, node_idx)) { /* Internal nodes */
                enum node_type node_type = get_node_type(node);
                if (node_type == NODE_TYPE_RN_SAM) {
                    fwk_assert(rnsam_entry < ctx->rnsam_count);

                    ctx->rnsam_table[rnsam_entry] = node;

                    rnsam_entry++;
                } else if (node_type == NODE_TYPE_HN_S) {
                    ldid = get_node_logical_id(node);
                    fwk_assert(ldid < ctx->hns_count);

                    ctx->hns_table[ldid].hns = (uintptr_t)(void *)node;

                    ctx->hns_table[ldid].node_pos.pos_x =
                        get_node_pos_x(get_node_id(node));

                    ctx->hns_table[ldid].node_pos.pos_y =
                        get_node_pos_y(get_node_id(node));

                    ctx->hns_table[ldid].node_pos.port_num = xp_port;

                    ctx->hns_table[ldid].node_pos.device_num =
                        get_device_number(get_node_id(node));

                    ctx->hns_table[ldid].xp = (uintptr_t)xp;
                } else if (node_type == NODE_TYPE_CCRA) {
                    ldid = get_node_logical_id(node);
                    fwk_assert(ldid < ctx->ccg_node_count);

                    /* Use ldid as index of the ccg_ra table */
                    ctx->ccg_ra_reg_table[ldid].node_id = node_id;
                    ctx->ccg_ra_reg_table[ldid].ccg_ra_reg =
                        (struct cmn_cyprus_ccg_ra_reg *)node;
                } else if (node_type == NODE_TYPE_CCHA) {
                    ldid = get_node_logical_id(node);
                    fwk_assert(ldid < ctx->ccg_node_count);

                    /* Use ldid as index of the ccg_ra table */
                    ctx->ccg_ha_reg_table[ldid].node_id = node_id;
                    ctx->ccg_ha_reg_table[ldid].ccg_ha_reg =
                        (struct cmn_cyprus_ccg_ha_reg *)node;
                } else if (node_type == NODE_TYPE_CCLA) {
                    ldid = get_node_logical_id(node);

                    /* Use ldid as index of the ccla table */
                    ctx->ccla_reg_table[ldid].node_id = node_id;
                    ctx->ccla_reg_table[ldid].ccla_reg =
                        (struct cmn_cyprus_ccla_reg *)node;
                }
            }
        }
    }
}

void cmn_cyprus_discovery(struct cmn_cyprus_ctx *ctx)
{
    int status;

    if (ctx->is_initialized) {
        return;
    }

    /* Traverse the mesh and discover the topology */
    status = discover_mesh_topology(ctx);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(MOD_NAME "Error! CMN Discovery failed");
        fwk_trap();
    }

    /*
     * Allocate resources based on the discovery.
     */

    /* RN-SAM node table */
    ctx->rnsam_table =
        fwk_mm_calloc(ctx->rnsam_count, sizeof(*ctx->rnsam_table));

    if (ctx->rnsam_table == NULL) {
        FWK_LOG_ERR(MOD_NAME "Error! Unable to allocate memory for ctx");
        fwk_trap();
    }

    /* HN-S node table */
    ctx->hns_table = fwk_mm_calloc(ctx->hns_count, sizeof(*ctx->hns_table));

    if (ctx->hns_table == NULL) {
        FWK_LOG_ERR(MOD_NAME "Error! Unable to allocate memory for ctx");
        fwk_trap();
    }

    /* Allocate resources for the CCG node descriptors in the context */
    if (ctx->ccg_node_count != 0) {
        ctx->ccg_ra_reg_table =
            fwk_mm_calloc(ctx->ccg_node_count, sizeof(*ctx->ccg_ra_reg_table));

        ctx->ccg_ha_reg_table =
            fwk_mm_calloc(ctx->ccg_node_count, sizeof(*ctx->ccg_ha_reg_table));

        ctx->ccla_reg_table =
            fwk_mm_calloc(ctx->ccg_node_count, sizeof(*ctx->ccla_reg_table));

        if ((ctx->ccg_ra_reg_table == NULL) ||
            (ctx->ccg_ha_reg_table == NULL) || (ctx->ccla_reg_table == NULL)) {
            FWK_LOG_ERR(MOD_NAME "Error! Unable to allocate memory for ctx");
            fwk_trap();
        }
    }

    /* Traverse the mesh and initialize context data */
    cmn_cyprus_init_ctx(ctx);

    FWK_LOG_INFO(MOD_NAME "CMN Discovery complete");
}
