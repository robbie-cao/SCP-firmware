/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lcp_mhu3.h>

#include <mod_dvfs_handler.h>
#include <mod_transport.h>

#include <fwk_assert.h>
#include <fwk_event.h>
#include <fwk_id.h>
#include <fwk_interrupt.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>

struct dvfs_handler_ctx {
    /* DVFS domain Id */
    fwk_id_t domain_id;

    /* Module configuration data */
    struct mod_dvfs_handler_config *config;

    /* Current perf level */
    uint32_t cur_level;

    /* Current perf limits */
    struct mod_dvfs_handler_perf_limits cur_limits;

    /* Number of operating points */
    size_t opp_count;

#ifdef BUILD_HAS_FAST_CHANNELS
    /* Access fastchannel */
    struct mod_transport_fast_channels_api *transport_fch_api;
#endif
};

struct mod_dvfs_handler_ctx {
    /* Number of DVFS domain */
    uint32_t dvfs_domain_element_count;

    /* DVFS device context table */
    struct dvfs_handler_ctx (*domain_ctx)[];
}dvfs_handler_ctx;

/*
 * Update the DVFS handler policy frequency register with the new frequency
 * in Hz.
 */
static inline void set_frequency(uint32_t idx, uint32_t new_freq)
{
    volatile dvfs_frame_t *dvfs_frame = (dvfs_frame_t *)
        (*dvfs_handler_ctx.domain_ctx)[idx].config->dvfs_handler_addr;

    if (dvfs_frame->policy_freq_st != new_freq) {
        dvfs_frame->policy_freq = new_freq;
        FWK_LOG_INFO("[DVFS HANDLER] Setting frequency to %luHz\n", new_freq);
    }
}

/*
 * Update the DVFS handler policy voltage register with the new voltage in micro
 * volts.
 */
static inline void set_voltage(uint32_t idx, uint32_t new_voltage)
{
    volatile dvfs_frame_t *dvfs_frame = (dvfs_frame_t *)
        (*dvfs_handler_ctx.domain_ctx)[idx].config->dvfs_handler_addr;

    if (dvfs_frame->policy_vlt_st != new_voltage) {
        dvfs_frame->policy_vlt = new_voltage;
        FWK_LOG_INFO("[DVFS HANDLER] Setting voltage to %luuV\n", new_voltage);
    }
}

/*
 * Check whether the Frequency and voltage settings are reflected in status
 * register.
 */
static int check_dvfs_status(uint32_t idx, uint32_t freq, uint32_t vlt)
{
    volatile dvfs_frame_t *dvfs_frame = (dvfs_frame_t *)
        (*dvfs_handler_ctx.domain_ctx)[idx].config->dvfs_handler_addr;

    if (dvfs_frame->policy_freq_st != freq) {
        FWK_LOG_WARN("[DVFS HANDLER] Failed to set frequency level\n");
        return FWK_PENDING;
    }

    if (dvfs_frame->policy_vlt_st != vlt) {
        FWK_LOG_WARN("[DVFS HANDLER] Failed to set voltage level\n");
        return FWK_PENDING;
    }

    return FWK_SUCCESS;
}

static size_t count_opps(const struct mod_dvfs_handler_opp *opps)
{
    const struct mod_dvfs_handler_opp *opp = &opps[0];

    while ((opp->level != 0) && (opp->voltage != 0) && (opp->frequency != 0)) {
        opp++;
    }

    return (size_t)(opp - &opps[0]);
}

/*
 * Get the opp table entry currespond to the requested performance level.
 */
static const struct mod_dvfs_handler_opp *get_opp_for_level(uint32_t idx,
    uint32_t level, bool use_nearest)
{
    struct mod_dvfs_handler_opp *opp_table;
    unsigned int opp_idx;
    struct dvfs_handler_ctx *ctx = &(*dvfs_handler_ctx.domain_ctx)[idx];

    opp_table = ctx->config->opps;

    if (use_nearest) {
        /*
         * Use nearest flag will ensure to return an opp table entry with in
         * the minimum and maximum range. Also if the requested level is not
         * supported by the dvfs handler, it will return an opp table entry
         * just below the requested level.
         */
        for (opp_idx = 0; opp_idx < ctx->opp_count; opp_idx++) {
            /* Navigate to opp table entry currespond to minimum limit */
            if (opp_table[opp_idx].level < ctx->cur_limits.minimum)
                continue;

            /*
             * Return minimum limit as requested level is less that or equal
             * minimum limit.
             */
            if (level <= opp_table[opp_idx].level)
                return &opp_table[opp_idx];

            /* Matching entry found or the search reached maximum limit */
            if ((opp_table[opp_idx].level == level) ||
                (opp_table[opp_idx].level == ctx->cur_limits.maximum))
                return &opp_table[opp_idx];

            /*
             * The opp level requested is not present in opp table. Return
             * a valid opp table entry just below the requested level.
             */
            if ((level < opp_table[opp_idx].level) &&
                (level > opp_table[opp_idx - 1].level))
                return &opp_table[opp_idx - 1];
        }
    } else {
        for (opp_idx = 0; opp_idx < ctx->opp_count; opp_idx++) {
            if (opp_table[opp_idx].level == level)
                return &opp_table[opp_idx];
        }
    }

    return NULL;
}

static int dvfs_handler_set_level(uint32_t idx, uint32_t level)
{
    const struct mod_dvfs_handler_opp *new_opp;
    int status;
    struct dvfs_handler_ctx *ctx = &(*dvfs_handler_ctx.domain_ctx)[idx];

    if (ctx->cur_level == level)
        return FWK_SUCCESS;

    /* Use a performance level with in limit */
    new_opp = get_opp_for_level(idx, level, true);
    if (new_opp == NULL) {
        FWK_LOG_ERR("[DVFS HANDLER] invalid level %lu\n", level);
        return FWK_E_RANGE;
    } else if (new_opp->level == ctx->cur_level) {
        return FWK_SUCCESS;
    }

    /* Program the hardware registers with frequency and voltage values */
    set_frequency(idx, new_opp->frequency);
    set_voltage(idx, new_opp->voltage);

    /* Check the status register whether the frequency and voltage changes are
     * reflected */
    status = check_dvfs_status(idx, new_opp->frequency, new_opp->voltage);
    if (status == FWK_SUCCESS)
        ctx->cur_level = new_opp->level;

    return status;
}

static int dvfs_handler_set_limit(uint32_t idx,
    struct mod_dvfs_handler_perf_limits limit)
{
    const struct mod_dvfs_handler_opp *new_opp;
    int status;
    struct dvfs_handler_ctx *ctx = &(*dvfs_handler_ctx.domain_ctx)[idx];

    if ((ctx->cur_limits.minimum == limit.minimum) &&
        (ctx->cur_limits.maximum == limit.maximum)) {
        /* Return success as minimum and maximum limits are unaltered */
        return FWK_SUCCESS;
    } else if ((limit.maximum == 0) && (limit.minimum == 0)) {
        return FWK_E_PARAM;
    } else if ((limit.maximum < limit.minimum) ||
        ((limit.maximum == 0) && (limit.minimum > ctx->cur_limits.maximum)) ||
        ((limit.minimum == 0) && (limit.maximum < ctx->cur_limits.minimum))) {
        /* Conditions checked:
         * 1. maximum limit less than minimum limit.
         * 2. When updating only minimum limit: The reset value of fastchannel
         *    is 0. When setting minimum limit alone after a reset, confirm the
         *    minimum limit is less than the current maximum limit.
         * 3. When updating only maximum limit: The reset value of fastchannel
         *    is 0. When setting maximum limit alone after a reset, confirm the
         *    maximum limit is greater than the current minimum limit.
         */
        return FWK_E_ALIGN;
    }

    /*
     * The reset value on fastchannel for minimum limit is zero. Ignore setting
     * minimum limit if the value is zero or same as the previous value, as the
     * fastchannel callback is invoked for set maximum limit.
     */
    if ((ctx->cur_limits.minimum != limit.minimum) && (limit.minimum != 0)) {
        new_opp = get_opp_for_level(idx, limit.minimum, false);
        if (new_opp == NULL) {
            FWK_LOG_ERR("[DVFS HANDLER] invalid mininum limit %lu\n",
                limit.minimum);
            return FWK_E_RANGE;
        }

        ctx->cur_limits.minimum = new_opp->level;

        /*
         * Check current perf level is with in limit, if not keep the level
         * with in limits.
         */
        if (ctx->cur_level < ctx->cur_limits.minimum) {
            status = dvfs_handler_set_level(idx, ctx->cur_limits.minimum);
            if (status != FWK_SUCCESS) {
                FWK_LOG_ERR("[DVFS HANDLER] Failed to set perf level above "
                    "minimum limit\n");
                return status;
            }
        }
    }

    /*
     * The reset value on fastchannel for maximum limit is zero. Ignore setting
     * maximum limit if the value is zero or same as the previous value, as the
     * fastchannel callback is invoked for set minimum limit.
     */
    if ((ctx->cur_limits.maximum != limit.maximum) && (limit.maximum != 0)) {
        new_opp = get_opp_for_level(idx, limit.maximum, false);
        if (new_opp == NULL) {
            FWK_LOG_ERR("[DVFS HANDLER] invalid maximum limit %lu\n",
                limit.maximum);
            return FWK_E_RANGE;
        }

        ctx->cur_limits.maximum = new_opp->level;

        /*
         * Check current perf level is with in limit, if not keep the level
         * with in limits.
         */
        if (ctx->cur_level > ctx->cur_limits.maximum) {
            status = dvfs_handler_set_level(idx, ctx->cur_limits.maximum);
            if (status != FWK_SUCCESS) {
                FWK_LOG_ERR("[DVFS HANDLER] Failed to set perf level below "
                    "maximum limit\n");
                return status;
            }
        }
    }

    return FWK_SUCCESS;
}

#ifdef BUILD_HAS_FAST_CHANNELS
/*
 * Callback function registered with transport module and there by with MHU3
 * module. This function will be invoked by the MHU3 module on receiving a
 * data over fastchannel for performance limit set.
 */
static void dvfs_fastchannel_set_limit_callback(uintptr_t dvfs_id)
{
    int status;
    uint32_t idx;
    struct fast_channel_addr fch;
    volatile struct mod_dvfs_handler_perf_limits *perf_limit;
    fwk_id_t id = *(fwk_id_t *)dvfs_id;
    struct dvfs_handler_ctx *ctx;

    idx = fwk_id_get_element_idx(id);
    ctx = &(*dvfs_handler_ctx.domain_ctx)[idx];

    status = ctx->transport_fch_api->transport_get_fch(
        ctx->config->dvfs_fch_set_limit_min, &fch);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[DVFS HANDLER] Failed to get fastchannel for performance"
           " limit set %d\n", status);
        return;
    }

    /* Read both minimum limit and maximum limit */
    perf_limit = (struct mod_dvfs_handler_perf_limits *)fch.local_view_address;
    status = dvfs_handler_set_limit(idx, *perf_limit);
    if (status == FWK_E_ALIGN) {
        FWK_LOG_WARN("[DVFS HANDLER] minimum limit greater than maximum\n");
        return;
    } else if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[DVFS HANDLER] Failed to set limits %d\n", status);
        return;
    }
}

/*
 * Callback function registered with transport module and there by with MHU3
 * module. This function will be invoked by the MHU3 module on receiving a
 * data over fastchannel for performance level set.
 */
static void dvfs_fastchannel_set_level_callback(uintptr_t dvfs_id)
{
    int status;
    uint32_t idx;
    struct fast_channel_addr fch;
    volatile uint32_t *perf_level;
    fwk_id_t id = *(fwk_id_t *)dvfs_id;
    struct dvfs_handler_ctx *ctx;

    idx = fwk_id_get_element_idx(id);
    ctx = &(*dvfs_handler_ctx.domain_ctx)[idx];

    status = ctx->transport_fch_api->transport_get_fch(
        ctx->config->dvfs_fch_set_level, &fch);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[DVFS HANDLER] Failed to get fastchannel for performance"
           " level set %d\n", status);
        return;
    }

    perf_level = (uint32_t *)fch.local_view_address;
    status = dvfs_handler_set_level(idx, *perf_level);
    if (status != FWK_SUCCESS)
        FWK_LOG_ERR("[DVFS HANDLER] Failed to set level %d\n", status);
}
#endif

static int get_current_opp(fwk_id_t domain_id,
    const struct mod_dvfs_handler_opp *opp)
{
    unsigned int idx = (unsigned int)fwk_id_get_element_idx(domain_id);
    struct dvfs_handler_ctx *ctx = &(*dvfs_handler_ctx.domain_ctx)[idx];

    opp = get_opp_for_level(idx, ctx->cur_level, false);
    if (opp == NULL)
        return FWK_E_PARAM;

    return FWK_SUCCESS;
}

static int get_sustained_opp(fwk_id_t domain_id,
    const struct mod_dvfs_handler_opp *opp)
{
    unsigned int idx = (unsigned int)fwk_id_get_element_idx(domain_id);
    struct dvfs_handler_ctx *ctx = &(*dvfs_handler_ctx.domain_ctx)[idx];

    opp = get_opp_for_level(idx, ctx->config->sustained_idx, false);
    if (opp == NULL)
        return FWK_E_PARAM;

    return FWK_SUCCESS;
}

static int get_opp_count(fwk_id_t domain_id, size_t *opp_count)
{
    unsigned int idx = (unsigned int)fwk_id_get_element_idx(domain_id);
    struct dvfs_handler_ctx *ctx = &(*dvfs_handler_ctx.domain_ctx)[idx];

    *opp_count = ctx->opp_count;
    return FWK_SUCCESS;
}

static int set_level(fwk_id_t domain_id, uint32_t level)
{
    unsigned int idx = (unsigned int)fwk_id_get_element_idx(domain_id);

    return dvfs_handler_set_level(idx, level);
}

static int set_limit(fwk_id_t domain_id,
    struct mod_dvfs_handler_perf_limits limits)
{
    unsigned int idx = (unsigned int)fwk_id_get_element_idx(domain_id);

    return dvfs_handler_set_limit(idx, limits);
}


struct mod_dvfs_handler_api dvfs_handler_mod_api = {
    .get_current_opp = get_current_opp,
    .get_sustained_opp = get_sustained_opp,
    .get_opp_count = get_opp_count,
    .set_level = set_level,
    .set_limit = set_limit,
};

static int dvfs_handler_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    dvfs_handler_ctx.domain_ctx = fwk_mm_calloc(element_count,
        sizeof((*dvfs_handler_ctx.domain_ctx)[0]));
    if (dvfs_handler_ctx.domain_ctx == NULL) {
        return FWK_E_NOMEM;
    }

    dvfs_handler_ctx.dvfs_domain_element_count = element_count;

    return FWK_SUCCESS;
}

static int dvfs_handler_element_init(
    fwk_id_t domain_id,
    unsigned int sub_element_count,
    const void *data)
{
    uint32_t idx = fwk_id_get_element_idx(domain_id);
    struct dvfs_handler_ctx *ctx = &(*dvfs_handler_ctx.domain_ctx)[idx];

    ctx->domain_id = domain_id;
    ctx->config = (struct mod_dvfs_handler_config *)data;

    if (ctx->config->opps == NULL) {
        return FWK_E_PARAM;
    }

    ctx->opp_count = count_opps(ctx->config->opps);
    if (ctx->opp_count <= 0) {
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

static int dvfs_handler_bind(fwk_id_t id, unsigned int round)
{
#ifdef BUILD_HAS_FAST_CHANNELS
    if (!fwk_id_is_type(id, FWK_ID_TYPE_ELEMENT)) {
        return FWK_SUCCESS;
    }

    int status;
    uint32_t idx = fwk_id_get_element_idx(id);

    status = fwk_module_bind(
        FWK_ID_ELEMENT(FWK_MODULE_IDX_TRANSPORT,
            MHU3_DEVICE_IDX_LCP_AP_FCH_DVFS_SET_LVL),
        FWK_ID_API(FWK_MODULE_IDX_TRANSPORT,
            MOD_TRANSPORT_API_IDX_FAST_CHANNELS),
            &(*dvfs_handler_ctx.domain_ctx)[idx].transport_fch_api);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[DVFS HANDLER] element%lu bind with transport failed %d\n",
            idx, status);
        return status;
    }
#endif

    return FWK_SUCCESS;
}

static int dvfs_handler_start(fwk_id_t id)
{
    if (!fwk_id_is_type(id, FWK_ID_TYPE_ELEMENT)) {
        return FWK_SUCCESS;
    }

    int status;
    uint32_t idx;
    uint32_t sustain_level;
    struct mod_dvfs_handler_opp *opp_table;
    struct dvfs_handler_ctx *ctx;

    idx = fwk_id_get_element_idx(id);
    ctx = &(*dvfs_handler_ctx.domain_ctx)[idx];
    opp_table = ctx->config->opps;
    sustain_level = opp_table[ctx->config->sustained_idx].level;

    ctx->cur_limits.minimum = opp_table[0].level;
    ctx->cur_limits.maximum = opp_table[ctx->opp_count - 1].level;

    status = dvfs_handler_set_level(idx, sustain_level);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[DVFS HANDLER] Failed to set sustained perf level\n");
        return status;
    }

#ifdef BUILD_HAS_FAST_CHANNELS
    /* Register callback function for performance level set */
    status = ctx->transport_fch_api-> transport_fch_register_callback(
        ctx->config->dvfs_fch_set_level,
        (uintptr_t)&ctx->domain_id,
        &dvfs_fastchannel_set_level_callback);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[DVFS HANDLER] Failed to register fastchannel callback"
           " for level set %d\n", status);
        return status;
    }

    /* Register same callback for minimum and maximum performance limit set */
    status = ctx->transport_fch_api-> transport_fch_register_callback(
            ctx->config->dvfs_fch_set_limit_min,
            (uintptr_t)&ctx->domain_id,
            &dvfs_fastchannel_set_limit_callback);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[DVFS HANDLER] Failed to register fastchannel callback"
           " for min limit set %d\n", status);
        return status;
    }

    status = ctx->transport_fch_api->transport_fch_register_callback(
        (*dvfs_handler_ctx.domain_ctx)[idx].config->dvfs_fch_set_limit_max,
        (uintptr_t)&ctx->domain_id,
        &dvfs_fastchannel_set_limit_callback);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[DVFS HANDLER] Failed to register fastchannel callback"
           " for max limit set %d\n", status);
        return status;
    }
#endif

    return FWK_SUCCESS;
}

static int dvfs_handler_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    enum mod_dvfs_handler_api_idx api_id_type =
        (enum mod_dvfs_handler_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_id_type) {
    case MOD_DVFS_HANDLER_API_IDX:
        *api = &dvfs_handler_mod_api;
        break;

    default:
        return FWK_E_PARAM;
    }
    return FWK_SUCCESS;
}

/* Module description */
const struct fwk_module module_dvfs_handler = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = MOD_DVFS_HANDLER_API_IDX_COUNT,
    .init = dvfs_handler_init,
    .element_init = dvfs_handler_element_init,
    .bind = dvfs_handler_bind,
    .start = dvfs_handler_start,
    .process_bind_request = dvfs_handler_process_bind_request,
};
