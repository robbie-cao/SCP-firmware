/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'transport'.
 */

#include "scp_clock.h"
#include "platform_core.h"
#include "scp_cfgd_mhu3.h"
#include "scp_cfgd_transport.h"
#include "scp_fw_mmap.h"

#include <mod_atu.h>
#include <mod_mhu3.h>
#include <mod_scp_platform.h>
#include <mod_transport.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <stdint.h>

static const struct fwk_element transport_element_table[] = {
    [SCP_CFGD_MOD_TRANSPORT_EIDX_PSCI] = {
        .name = "PSCI",
        .data = &((
            struct mod_transport_channel_config) {
                .transport_type = MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_OUT_BAND,
                .policies = MOD_TRANSPORT_POLICY_INIT_MAILBOX |
                    MOD_TRANSPORT_POLICY_SECURE,
                .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
                .out_band_mailbox_address =
                    (uintptr_t) SCP_SCMI_PAYLOAD_S_A2P_BASE,
            .out_band_mailbox_size = SCP_SCMI_PAYLOAD_SIZE,
            .driver_id =
                FWK_ID_SUB_ELEMENT_INIT(
                    FWK_MODULE_IDX_MHU3,
                    SCP_CFGD_MOD_MHU3_EIDX_SCP_AP_S_CLUS0,
                    0),
            .driver_api_id =
                FWK_ID_API_INIT(
                    FWK_MODULE_IDX_MHU3,
                    MOD_MHU3_API_IDX_TRANSPORT_DRIVER),
        }),
    },
    [SCP_CFGD_MOD_TRANSPORT_EIDX_ATU] = {
        .name = "ATU_TRANSPORT",
        .data = &((
            struct mod_transport_channel_config) {
                .transport_type = MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_OUT_BAND,
                .policies = MOD_TRANSPORT_POLICY_INIT_MAILBOX |
		    MOD_TRANSPORT_POLICY_SECURE,
                .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_REQUESTER,
                .out_band_mailbox_address =
		    (uintptr_t) SCP_ATU_TRANSPORT_PAYLOAD_BASE,
                .out_band_mailbox_size = SCP_ATU_TRANSPORT_PAYLOAD_SIZE,
                .signal_api_id =
                    FWK_ID_API_INIT(
                        FWK_MODULE_IDX_ATU,
                        MOD_ATU_API_IDX_TRANSPORT_SIGNAL),
                .driver_id =
                    FWK_ID_SUB_ELEMENT_INIT(
                        FWK_MODULE_IDX_MHU3,
                        SCP_CFGD_MOD_MHU3_EIDX_SCP_RSS_S,
                        0),
                .driver_api_id =
                    FWK_ID_API_INIT(
                        FWK_MODULE_IDX_MHU3,
                        MOD_MHU3_API_IDX_TRANSPORT_DRIVER),
        }),
    },
    [SCP_CFGD_MOD_TRANSPORT_EIDX_SYSTEM] = {
        .name = "SCP_PLATFORM_TRANSPORT",
        .data = &((
            struct mod_transport_channel_config) {
                .transport_type = MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_NONE,
                .policies = MOD_TRANSPORT_POLICY_NONE,
                .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
                .signal_api_id =
                    FWK_ID_API_INIT(
                        FWK_MODULE_IDX_SCP_PLATFORM,
                        MOD_SCP_PLATFORM_API_IDX_TRANSPORT_SIGNAL),
                .driver_id =
                    FWK_ID_SUB_ELEMENT_INIT(
                        FWK_MODULE_IDX_MHU3,
                        SCP_CFGD_MOD_MHU3_EIDX_SCP_RSS_S,
                        1),
                .driver_api_id =
                    FWK_ID_API_INIT(
                        FWK_MODULE_IDX_MHU3,
                        MOD_MHU3_API_IDX_TRANSPORT_DRIVER),
        }),
    },
    [SCP_CFGD_MOD_TRANSPORT_EIDX_RESET] = {
        .name = "SCP_PLATFORM_TRANSPORT_RESET",
        .data = &((
            struct mod_transport_channel_config) {
            .transport_type = MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_NONE,
            .policies = MOD_TRANSPORT_POLICY_NONE,
            .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_REQUESTER,
            .signal_api_id =
                FWK_ID_API_INIT(
                    FWK_MODULE_IDX_SCP_PLATFORM,
                    MOD_SCP_PLATFORM_API_IDX_TRANSPORT_SIGNAL),
            .driver_id =
                FWK_ID_SUB_ELEMENT_INIT(
                    FWK_MODULE_IDX_MHU3,
                    SCP_CFGD_MOD_MHU3_EIDX_SCP_RSS_S,
                    2),
            .driver_api_id =
                FWK_ID_API_INIT(
                    FWK_MODULE_IDX_MHU3,
                    MOD_MHU3_API_IDX_TRANSPORT_DRIVER),
        }),
    },
    [SCP_CFGD_MOD_TRANSPORT_EIDX_COUNT] = { 0 },
};

static const struct fwk_element *transport_get_element_table(fwk_id_t module_id)
{
    struct mod_transport_channel_config *config;
    unsigned int idx;

    for (idx = 0; idx < SCP_CFGD_MOD_TRANSPORT_EIDX_COUNT; idx++) {
        config =
            (struct mod_transport_channel_config *)(transport_element_table[idx]
                                                        .data);
        config->clock_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_CLOCK,
            CFGD_MOD_CLOCK_EIDX_CMN);
    }

    return transport_element_table;
}

const struct fwk_module_config config_transport = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(transport_get_element_table),
};
