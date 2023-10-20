/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Driver module for CMN Cyprus interconnect.
 */

#include <internal/cmn_cyprus_ccg.h>
#include <internal/cmn_cyprus_ctx.h>
#include <internal/cmn_cyprus_discovery.h>
#include <internal/cmn_cyprus_hnsam.h>
#include <internal/cmn_cyprus_reg.h>
#include <internal/cmn_cyprus_rnsam.h>

#include <mod_clock.h>
#include <mod_cmn_cyprus.h>
#include <mod_system_info.h>
#include <mod_timer.h>

#include <fwk_assert.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_notification.h>
#include <fwk_status.h>

#include <inttypes.h>
#include <stdint.h>

/* Max Mesh size */
#define CMN_CYPRUS_MESH_X_MAX 12
#define CMN_CYPRUS_MESH_Y_MAX 12

/* Module context */
static struct cmn_cyprus_ctx *ctx;

/* Configure the mesh */
static int cmn_cyprus_setup(void)
{
    /* Discover the mesh and setup the context data */
    cmn_cyprus_discovery(ctx);

    /* Program the HN-F SAM */
    cmn_cyprus_setup_hnf_sam(ctx);

    /* Program the RN SAM */
    cmn_cyprus_setup_rnsam(ctx);

    /* Program the CCG and enable CML */
    cmn_cyprus_setup_cml(ctx);

    return FWK_SUCCESS;
}

static int validate_config_data()
{
    struct mod_cmn_cyprus_config *config =
        &ctx->config_table->chip_config_data[ctx->chip_id];

    if (config->base == 0) {
        return FWK_E_DATA;
    }

    if ((config->mesh_size_x == 0) ||
        (config->mesh_size_x > CMN_CYPRUS_MESH_X_MAX)) {
        return FWK_E_DATA;
    }

    if ((config->mesh_size_y == 0) ||
        (config->mesh_size_y > CMN_CYPRUS_MESH_Y_MAX)) {
        return FWK_E_DATA;
    }

    return FWK_SUCCESS;
}

/* Framework handlers */
static int cmn_cyprus_init(
    fwk_id_t module_id,
    unsigned int unused,
    const void *data)
{
    struct mod_cmn_cyprus_config_table *config_table;

    fwk_assert(data != NULL);

    config_table = (struct mod_cmn_cyprus_config_table *)data;

    /* Allocate memory for the module context table */
    ctx = fwk_mm_calloc(1, sizeof(struct cmn_cyprus_ctx));

    ctx->config_table = config_table;

    return FWK_SUCCESS;
}

static int cmn_cyprus_bind(fwk_id_t id, unsigned int round)
{
    /* Use second round only (round numbering is zero-indexed) */
    if (round == 1) {
        /* Bind to the timer component */
        return fwk_module_bind(
            FWK_ID_ELEMENT(FWK_MODULE_IDX_TIMER, 0),
            FWK_ID_API(FWK_MODULE_IDX_TIMER, MOD_TIMER_API_IDX_TIMER),
            &ctx->timer_api);
    }

    /* Bind to system info module to obtain multi-chip info */
    return fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SYSTEM_INFO),
        FWK_ID_API(FWK_MODULE_IDX_SYSTEM_INFO, MOD_SYSTEM_INFO_GET_API_IDX),
        &ctx->system_info_api);
}

int cmn_cyprus_start(fwk_id_t id)
{
    int status;
    /* Structure used to store the system info from system info API */
    const struct mod_system_info *system_info;

    status = ctx->system_info_api->get_system_info(&system_info);
    if (status != FWK_SUCCESS) {
        return status;
    }

    ctx->chip_id = system_info->chip_id;

    FWK_LOG_INFO(
        MOD_NAME "Multichip mode: %s",
        system_info->multi_chip_mode ? "Enabled" : "Disabled");
    FWK_LOG_INFO(MOD_NAME "Chip ID: %d", ctx->chip_id);

    if (ctx->chip_id > ctx->config_table->chip_count) {
        FWK_LOG_ERR(
            MOD_NAME "No config data available for chip %u", ctx->chip_id);
        return FWK_E_DATA;
    }

    status = validate_config_data();
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Initialize the chip-specific config data in the context */
    ctx->config = &ctx->config_table->chip_config_data[ctx->chip_id];

    ctx->root = (struct cmn_cyprus_cfgm_reg *)ctx->config->base;

    if (fwk_id_is_equal(ctx->config->clock_id, FWK_ID_NONE)) {
        return cmn_cyprus_setup();
    }

    /* Register the module for clock state notifications */
    return fwk_notification_subscribe(
        mod_clock_notification_id_state_changed, ctx->config->clock_id, id);
}

static int cmn_cyprus_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    struct clock_notification_params *params;

    fwk_assert(
        fwk_id_is_equal(event->id, mod_clock_notification_id_state_changed));

    params = (struct clock_notification_params *)event->params;

    if (params->new_state == MOD_CLOCK_STATE_RUNNING) {
        return cmn_cyprus_setup();
    }

    return FWK_SUCCESS;
}

static int cmn_cyprus_process_bind_request(
    fwk_id_t requester_id,
    fwk_id_t id,
    fwk_id_t api_id,
    const void **api)
{
    enum mod_cmn_cyprus_api_idx api_idx;
    int status;

    api_idx = (enum mod_cmn_cyprus_api_idx)fwk_id_get_api_idx(api_id);

    /* Invalid parameters */
    if ((api == NULL) || (!fwk_module_is_valid_module_id(id))) {
        return FWK_E_PARAM;
    }

    switch (api_idx) {
    case MOD_CMN_CYPRUS_API_IDX_MAP_IO_REGION:
        get_rnsam_memmap_api(api);
        status = FWK_SUCCESS;
        break;

    default:
        status = FWK_E_PARAM;
        break;
    };

    return status;
}

const struct fwk_module module_cmn_cyprus = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = MOD_CMN_CYPRUS_API_COUNT,
    .init = cmn_cyprus_init,
    .bind = cmn_cyprus_bind,
    .start = cmn_cyprus_start,
    .process_notification = cmn_cyprus_process_notification,
    .process_bind_request = cmn_cyprus_process_bind_request,
};
