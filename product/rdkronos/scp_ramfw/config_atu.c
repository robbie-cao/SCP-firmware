/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'atu'.
 */

#include "scp_cfgd_transport.h"

#include <mod_atu.h>
#include <mod_transport.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>

static const struct fwk_element element_table[] = {
    [0] = {
        .name = "SCP_ATU",
        .data = &(struct mod_atu_device_config) {
            .is_atu_delegated = true,
            .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .transport_id =
                FWK_ID_ELEMENT_INIT(
                    FWK_MODULE_IDX_TRANSPORT,
                    SCP_CFGD_MOD_TRANSPORT_EIDX_ATU),
        },
    },
    [1] = { 0 },
};

static const struct fwk_element *get_element_table(fwk_id_t module_id)
{
    return element_table;
}

struct fwk_module_config config_atu = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_element_table),
};
