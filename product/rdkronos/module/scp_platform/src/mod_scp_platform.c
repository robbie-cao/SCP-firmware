/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SCP sub-system support.
 */

#include "scp_clock.h"
#include "core_manager.h"
#include "platform_core.h"
#include "scp_cfgd_sds.h"
#include "scp_css_mmap.h"
#include "scp_pwrctrl.h"
#include "scp_cfgd_scmi.h"
#include "scp_cfgd_transport.h"

#include <mod_clock.h>
#include <mod_scp_platform.h>
#include <mod_power_domain.h>
#include <mod_ppu_v1.h>
#include <mod_scmi.h>
#include <mod_sds.h>
#include <mod_system_info.h>
#include <mod_system_power.h>
#include <mod_timer.h>
#include <mod_transport.h>

#include <fwk_assert.h>
#include <fwk_event.h>
#include <fwk_id.h>
#include <fwk_interrupt.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_notification.h>
#include <fwk_status.h>

#include <fmw_cmsis.h>

#include <stdbool.h>
#include <stdint.h>

#define MOD_NAME "[SCP_PLATFORM]"

/* Timeout value for the timer API wait function */
#define RSS_DOORBELL_WAIT_TIMEOUT_US (500 * 1000)

/* SRAM address where TF-A BL2 binary will be preloaded in the FVP */
#define ARM_TF_BL2_SRAM_ADDR (0x00042000)

static const struct mod_system_info *system_info;

/* Flag to indicate that the RSS initialization is complete */
static volatile bool rss_init_done = false;

/* Timer identifier to which the scp platform module must bind to */
static fwk_id_t timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0);

/* Transport channel identifier. Used for receiving events from RSS */
static fwk_id_t transport_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TRANSPORT,
    SCP_CFGD_MOD_TRANSPORT_EIDX_SYSTEM);

fwk_id_t pd_transition_notification_id =
    FWK_ID_NOTIFICATION_INIT(FWK_MODULE_IDX_POWER_DOMAIN,
                        MOD_PD_NOTIFICATION_IDX_POWER_STATE_TRANSITION);

/* Transport channel identifier. Used for sending reset events to RSS */
static fwk_id_t reset_ch_transport_id = FWK_ID_ELEMENT_INIT(
    FWK_MODULE_IDX_TRANSPORT, SCP_CFGD_MOD_TRANSPORT_EIDX_RESET);

static fwk_id_t transport_id_clus0 = FWK_ID_ELEMENT_INIT(
    FWK_MODULE_IDX_TRANSPORT, SCP_CFGD_MOD_TRANSPORT_EIDX_BOOT_SI_CLUS0);

static fwk_id_t transport_id_clus1 = FWK_ID_ELEMENT_INIT(
    FWK_MODULE_IDX_TRANSPORT, SCP_CFGD_MOD_TRANSPORT_EIDX_BOOT_SI_CLUS1);

static fwk_id_t transport_id_clus2 = FWK_ID_ELEMENT_INIT(
    FWK_MODULE_IDX_TRANSPORT, SCP_CFGD_MOD_TRANSPORT_EIDX_BOOT_SI_CLUS2);

/* SCMI services required to enable the messaging stack */
static unsigned int scmi_notification_table[] = {
    SCP_CFGD_MOD_SCMI_EIDX_PSCI,
};

/* Module context */
struct scp_platform_ctx {
    /* Pointer to the SCP Power Control register block */
    struct scp_power_control_reg *scp_pwrctrl_reg;

    /* Pointer to the Interrupt Service Routine API of the PPU_V1 module */
    const struct ppu_v1_isr_api *ppu_v1_isr_api;

    /* Power domain module restricted API pointer */
    struct mod_pd_restricted_api *mod_pd_restricted_api;

    /*! Transport API to send/respond to a message */
    const struct mod_transport_firmware_api *transport_api;

    /* SDS module API pointer */
    const struct mod_sds_api *sds_api;

    /*! Timer API */
    const struct mod_timer_api *timer_api;

    /* System Information HAL API pointer */
    struct mod_system_info_get_info_api *system_info_api;
};

static struct scp_platform_ctx scp_platform_ctx;

struct scp_platform_isr {
    unsigned int interrupt;
    void (*handler)(void);
};

static const uint32_t feature_flags = PLATFORM_SDS_FEATURE_FIRMWARE_MASK;

static fwk_id_t sds_feature_availability_id =
    FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_SDS,
        SCP_CFGD_MOD_SDS_EIDX_FEATURE_AVAILABILITY);

/* Utility function to check RSS init status */
bool check_rss_init_status(void *unused)
{
    /*
     * Check if the rss_init flag is set. This flag will be set by the
     * signal_message() function when SCP receives doorbell from RSS.
     */
    if (rss_init_done) {
        FWK_LOG_ERR("[SCP_PLATFORM] RSS initialized!\n");
    }
    return rss_init_done;
}

/* Helper function to program the AP Core RVBAR */
void program_ap_rvbar()
{
    uint8_t core_idx;

    for (core_idx = 0; core_idx < platform_get_core_count(); core_idx++) {
        /* Set RVBAR to TF-A BL2 SRAM address */
        SCP_CLUSTER_UTILITY_CORE_MANAGER_PTR(core_idx)->PE_RVBARADDR_LW = ARM_TF_BL2_SRAM_ADDR;
        SCP_CLUSTER_UTILITY_CORE_MANAGER_PTR(core_idx)->PE_RVBARADDR_UP = 0;
    }
}

/* Helper function to program the LCP UART access */
void enable_lcp_uart(uint8_t lcp_idx)
{
    volatile uint32_t *lcp_uart_ctrl_reg;

    /*
     * Allow LCP to access the UART.
     */
    lcp_uart_ctrl_reg =
        (volatile uint32_t *)(SCP_LCP_EXTERNAL_CONTROL(lcp_idx) + 0x020);

    *lcp_uart_ctrl_reg |= (0x1);
}

/* Helper function to release LCPs */
void release_lcp()
{
    volatile uint32_t *cpu_wait;
    uint8_t lcp_idx = 0;

    /*
     * Allow LCP0 to access the UART. If all the LCPs are allowed to access
     * the UART at the same time, the output will be unreadable. Hence,
     * restrict the LCP UART to single LCP for now.
     */
    enable_lcp_uart(lcp_idx);

    /* Release all the LCPs */
    for (; lcp_idx < platform_get_core_count(); lcp_idx++) {
        cpu_wait = (volatile uint32_t *)(SCP_LCP_CONTROL(lcp_idx) +
                SCP_LCP_CONTROL_CPU_WAIT_OFFSET);
        *cpu_wait &= ~1;
    }
}

/*
 *  SCMI Messaging stack
 */
static int messaging_stack_ready(void)
{
    const struct mod_sds_structure_desc *sds_structure_desc =
        fwk_module_get_data(sds_feature_availability_id);

    /*
     * Write SDS Feature Availability to signal that the messaging
     * stack is ready.
     */
    return scp_platform_ctx.sds_api->struct_write(
        sds_structure_desc->id,
        0,
        (void *)(&feature_flags),
        sds_structure_desc->size);
}

/*
 *  PPU Interrupt Service Routines for cluster and core power domains
 */
static void ppu_cores_isr(unsigned int first, uint32_t status)
{
    unsigned int core_idx;

    while (status != 0) {
        core_idx = __builtin_ctz(status);
        status &= ~(1 << core_idx);

        if ((first + core_idx) >= platform_get_core_count()) {
            continue;
        }

        scp_platform_ctx.ppu_v1_isr_api->ppu_interrupt_handler(
            FWK_ID_ELEMENT(FWK_MODULE_IDX_PPU_V1, first + core_idx));
    }
}

static void ppu_cores_isr_0(void)
{
    ppu_cores_isr(0, scp_platform_ctx.scp_pwrctrl_reg->CPU_PPU_INT_STATUS[0]);
}

static void ppu_cores_isr_1(void)
{
    ppu_cores_isr(32, scp_platform_ctx.scp_pwrctrl_reg->CPU_PPU_INT_STATUS[1]);
}

static void ppu_cores_isr_2(void)
{
    ppu_cores_isr(64, scp_platform_ctx.scp_pwrctrl_reg->CPU_PPU_INT_STATUS[2]);
}

static void ppu_cores_isr_3(void)
{
    ppu_cores_isr(96, scp_platform_ctx.scp_pwrctrl_reg->CPU_PPU_INT_STATUS[3]);
}

static void ppu_clusters_isr(void)
{
    uint32_t status = scp_platform_ctx.scp_pwrctrl_reg->CLUS_PPU_INT_STATUS[0];
    unsigned int cluster_idx;

    while (status != 0) {
        cluster_idx = __builtin_ctz(status);

        scp_platform_ctx.ppu_v1_isr_api->ppu_interrupt_handler(
            FWK_ID_ELEMENT(
                FWK_MODULE_IDX_PPU_V1,
                platform_get_core_count() + cluster_idx));

        status &= ~(1 << cluster_idx);
    }
}

/*
 *  PPU Interrupt Service Routine table
 */
static struct scp_platform_isr isrs[] = {
    [0] = { .interrupt = PPU_CORES0_IRQ, .handler = ppu_cores_isr_0 },
    [1] = { .interrupt = PPU_CORES1_IRQ, .handler = ppu_cores_isr_1 },
    [2] = { .interrupt = PPU_CORES2_IRQ, .handler = ppu_cores_isr_2 },
    [3] = { .interrupt = PPU_CORES3_IRQ, .handler = ppu_cores_isr_3 },
    [4] = { .interrupt = PPU_CLUSTERS0_IRQ, .handler = ppu_clusters_isr },
};

/*
 * System power module driver API
 */
static int scp_platform_shutdown(enum mod_pd_system_shutdown system_shutdown)
{
    int status;

    status = scp_platform_ctx.transport_api->trigger_interrupt(
            reset_ch_transport_id);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[PLATFORM_SYSTEM] FATAL ERROR! Unable to trigger"
                "RSS doorbell for reset event\n");
        return FWK_E_PANIC;
    }

    /* wait for RSS to complete the system wide reset */
    __WFI();

    return FWK_E_DEVICE;
}

static const struct mod_system_power_driver_api
    scp_platform_system_power_driver_api = {
        .system_shutdown = scp_platform_shutdown,
};

/*
 * Transport signal API implementation
 */
static int signal_error(fwk_id_t unused)
{
    FWK_LOG_INFO("[SCP_PLATFORM] ERROR in the received message!\n");

    scp_platform_ctx.transport_api->release_transport_channel_lock(
        transport_id);

    return FWK_SUCCESS;
}

static int signal_message(fwk_id_t unused)
{
    FWK_LOG_INFO("[SCP_PLATFORM] Received doorbell event!\n");

    scp_platform_ctx.transport_api->release_transport_channel_lock(
        transport_id);

    /* Set the flag to indicate that the RSS initialization is complete */
    rss_init_done = true;

    return FWK_SUCCESS;
}

static int si_power_on_cluster_cores(uint32_t id, uint32_t num, uint32_t offset)
{
    int status;
    uint32_t core;
    struct mod_pd_restricted_api *mod_pd_restricted_api =
                            scp_platform_ctx.mod_pd_restricted_api;
    uint32_t start_id = platform_get_core_count() +
                        platform_get_cluster_count() + offset;

    for(core = 0; core < num; core++){
        status = mod_pd_restricted_api->set_state(
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_DOMAIN, start_id + core),
        false,
        MOD_PD_COMPOSITE_STATE(
        MOD_PD_LEVEL_1,
        0,
        MOD_PD_STATE_ON,
        MOD_PD_STATE_ON,
        MOD_PD_STATE_ON));

        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(
                "[PLATFORM SYSTEM] Failed to intialize the SI cluster%ld core%ld", id, core);
            return status;
        }
    }

    return FWK_SUCCESS;
}

static int signal_message_boot_si_cl0(fwk_id_t unused)
{
    FWK_LOG_INFO("[PLATFORM_SYSTEM] Received si cl0 doorbell event!\n");

    return si_power_on_cluster_cores(SCP_SI_CL0_ID, SCP_SI_CL0_CORE_NUM,
                                     SCP_SI_CL0_CORE_OFS);
}

static int signal_message_boot_si_cl1(fwk_id_t unused)
{
    FWK_LOG_INFO("[PLATFORM_SYSTEM] Received si cl1 doorbell event!\n");

    return si_power_on_cluster_cores(SCP_SI_CL1_ID, SCP_SI_CL1_CORE_NUM,
                                     SCP_SI_CL1_CORE_OFS);
}

static int signal_message_boot_si_cl2(fwk_id_t unused)
{
    FWK_LOG_INFO("[PLATFORM_SYSTEM] Received si cl2 doorbell event!\n");

    return si_power_on_cluster_cores(SCP_SI_CL2_ID, SCP_SI_CL2_CORE_NUM,
                                     SCP_SI_CL2_CORE_OFS);
}

static const struct mod_transport_firmware_signal_api transport_signal_api = {
    .signal_error = signal_error,
    .signal_message = signal_message,
};

static const struct mod_transport_firmware_signal_api transport_boot_si_cl0 = {
    .signal_error = signal_error,
    .signal_message = signal_message_boot_si_cl0,
};

static const struct mod_transport_firmware_signal_api transport_boot_si_cl1 = {
    .signal_error = signal_error,
    .signal_message = signal_message_boot_si_cl1,
};

static const struct mod_transport_firmware_signal_api transport_boot_si_cl2 = {
    .signal_error = signal_error,
    .signal_message = signal_message_boot_si_cl2,
};

/*
 * Framework handlers
 */
static int scp_platform_mod_init(
    fwk_id_t module_id,
    unsigned int unused,
    const void *unused2)
{
    int status;
    unsigned int idx;
    struct scp_platform_isr *isr;

    for (idx = 0; idx < FWK_ARRAY_SIZE(isrs); idx++) {
        isr = &isrs[idx];

        status = fwk_interrupt_set_isr(isr->interrupt, isr->handler);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    scp_platform_ctx.scp_pwrctrl_reg =
        (struct scp_power_control_reg *)SCP_POWER_CONTROL_BASE;

    return FWK_SUCCESS;
}

static int scp_platform_bind(fwk_id_t id, unsigned int round)
{
    int status;

    if (round > 0) {
        return FWK_SUCCESS;
    }

    fwk_id_t timer_api_id = FWK_ID_API_INIT(
        FWK_MODULE_IDX_TIMER, MOD_TIMER_API_IDX_TIMER);

    /* Bind to timer API */
    status = fwk_module_bind(
        timer_id, timer_api_id, &scp_platform_ctx.timer_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Bind to transport module firmware api */
    status = fwk_module_bind(
        transport_id,
        FWK_ID_API(FWK_MODULE_IDX_TRANSPORT, MOD_TRANSPORT_API_IDX_FIRMWARE),
        &scp_platform_ctx.transport_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Bind to transport module firmware api */
    status = fwk_module_bind(
        reset_ch_transport_id,
        FWK_ID_API(FWK_MODULE_IDX_TRANSPORT, MOD_TRANSPORT_API_IDX_FIRMWARE),
        &scp_platform_ctx.transport_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_module_bind(
        transport_id_clus0,
        FWK_ID_API(FWK_MODULE_IDX_TRANSPORT, MOD_TRANSPORT_API_IDX_FIRMWARE),
        &scp_platform_ctx.transport_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_module_bind(
        transport_id_clus1,
        FWK_ID_API(FWK_MODULE_IDX_TRANSPORT, MOD_TRANSPORT_API_IDX_FIRMWARE),
        &scp_platform_ctx.transport_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_module_bind(
        transport_id_clus2,
        FWK_ID_API(FWK_MODULE_IDX_TRANSPORT, MOD_TRANSPORT_API_IDX_FIRMWARE),
        &scp_platform_ctx.transport_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Bind to Power Domain module restricted API */
    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_POWER_DOMAIN),
        FWK_ID_API(FWK_MODULE_IDX_POWER_DOMAIN, MOD_PD_API_IDX_RESTRICTED),
        &scp_platform_ctx.mod_pd_restricted_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Bind to PPUv1 driver module ISR API */
    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_PPU_V1),
        FWK_ID_API(FWK_MODULE_IDX_PPU_V1, MOD_PPU_V1_API_IDX_ISR),
        &scp_platform_ctx.ppu_v1_isr_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Bind to System Info API */
    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SYSTEM_INFO),
        FWK_ID_API(FWK_MODULE_IDX_SYSTEM_INFO, MOD_SYSTEM_INFO_GET_API_IDX),
        &scp_platform_ctx.system_info_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Bind to SDS module API */
    return fwk_module_bind(
        fwk_module_id_sds,
        FWK_ID_API(FWK_MODULE_IDX_SDS, 0),
        &scp_platform_ctx.sds_api);
}

static int scp_platform_process_bind_request(
    fwk_id_t requester_id,
    fwk_id_t pd_id,
    fwk_id_t api_id,
    const void **api)
{
    enum mod_scp_platform_api_idx api_id_type;

    api_id_type = (enum mod_scp_platform_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_id_type) {
    case MOD_SCP_PLATFORM_API_IDX_SYSTEM_POWER_DRIVER:
        *api = &scp_platform_system_power_driver_api;
        break;

    case MOD_SCP_PLATFORM_API_IDX_TRANSPORT_SIGNAL:
        *api = &transport_signal_api;
        break;

    case MOD_SCP_PLATFORM_API_IDX_BOOT_SI_CLUS0:
        *api = &transport_boot_si_cl0;
        break;

    case MOD_SCP_PLATFORM_API_IDX_BOOT_SI_CLUS1:
        *api = &transport_boot_si_cl1;
        break;

    case MOD_SCP_PLATFORM_API_IDX_BOOT_SI_CLUS2:
        *api = &transport_boot_si_cl2;
        break;

    default:
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

static int scp_platform_start(fwk_id_t id)
{
    int status;
    unsigned int i;

    status = scp_platform_ctx.system_info_api->get_system_info(&system_info);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[SCP PLATFORM] Failed to obtain system information");
        return status;
    }

        /*
         * Subscribe to the clock notification to know when the interconnect
         * clock state changes.
         */
        status = fwk_notification_subscribe(
            mod_clock_notification_id_state_changed,
            FWK_ID_ELEMENT(FWK_MODULE_IDX_CLOCK, CFGD_MOD_CLOCK_EIDX_CMN),
            id);
        if (status != FWK_SUCCESS) {
            return status;
        }

        fwk_id_t pd_transition_source_id = fwk_id_build_element_id(
                fwk_module_id_power_domain,
                platform_get_core_count() + platform_get_cluster_count() +
                0);

        fwk_notification_subscribe(pd_transition_notification_id,
                pd_transition_source_id, id);

    if (system_info->chip_id == 0) {
        /*
         * Subscribe to the SDS initialized notification so that we can let the
         * PSCI agent know that the SCMI stack is initialized.
         */
        status = fwk_notification_subscribe(
            mod_sds_notification_id_initialized, fwk_module_id_sds, id);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }
    /*
     * Subscribe to the SCMI channel(s) in order to know when the channel
     * gets initialized.
     */
    for (i = 0; i < FWK_ARRAY_SIZE(scmi_notification_table); i++) {
        status = fwk_notification_subscribe(
            mod_scmi_notification_id_initialized,
            fwk_id_build_element_id(
                fwk_module_id_scmi, scmi_notification_table[i]),
            id);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    FWK_LOG_INFO("%s Requesting SYSTOP initialization...\n", MOD_NAME);
    status = scp_platform_ctx.mod_pd_restricted_api->set_state(
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_DOMAIN, 0),
        false,
        MOD_PD_COMPOSITE_STATE(
            MOD_PD_LEVEL_2,
            0,
            MOD_PD_STATE_ON,
            MOD_PD_STATE_OFF,
            MOD_PD_STATE_OFF));

    return status;
}

int scp_platform_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    int status;
    struct clock_notification_params *params;
    struct mod_pd_restricted_api *mod_pd_restricted_api;
    static unsigned int scmi_notification_count = 0;
    static bool sds_notification_received = false;

    fwk_assert(fwk_id_is_type(event->target_id, FWK_ID_TYPE_MODULE));

    if (fwk_id_is_equal(event->id, pd_transition_notification_id)) {
        /*
         * Trigger RSS doorbell to indicate that the SYSTOP domain is ON.
         */
        status = scp_platform_ctx.transport_api->trigger_interrupt(
                transport_id);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR("[SCP_PLATFORM] FATAL ERROR! Unable to trigger"
                    "RSS doorbell\n");
            return FWK_E_PANIC;
        }
        /*
         * Wait till a doorbell from RSS is received. This doorbell event indicates
         * that the RSS LCS is in secure state and the RSS has completed loading
         * the LCP ramfw images to LCP ITCM.
         */
        status = scp_platform_ctx.timer_api->wait(timer_id,
                RSS_DOORBELL_WAIT_TIMEOUT_US, check_rss_init_status, NULL);

        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR("[SCP_PLATFORM] FATAL ERROR! Timed out."
                    "No response from RSS\n");
            return FWK_E_PANIC;
        }
    } else if (fwk_id_is_equal(event->id, mod_clock_notification_id_state_changed)) {
        params = (struct clock_notification_params *)event->params;

        program_ap_rvbar();

        /*
         * Initialize primary core and LCPs
         */
        if (params->new_state == MOD_CLOCK_STATE_RUNNING) {
            /* Configure LCP0 UART access and release all LCPs */
            release_lcp();

            if (system_info->chip_id == 0) {
                FWK_LOG_INFO(
                        "[SCP PLATFORM] Initializing the primary core...\n");

                mod_pd_restricted_api =
                    scp_platform_ctx.mod_pd_restricted_api;

                status = mod_pd_restricted_api->set_state(
                    FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_DOMAIN, 0),
                    false,
                    MOD_PD_COMPOSITE_STATE(
                        MOD_PD_LEVEL_2,
                        0,
                        MOD_PD_STATE_ON,
                        MOD_PD_STATE_ON,
                        MOD_PD_STATE_ON));

                if (status != FWK_SUCCESS) {
                    FWK_LOG_ERR(
                            "[SCP PLATFORM] "
                            "Failed to initialize the primary core\n");
                    return status;
                }

                /* Unsubscribe to the notification */
                return fwk_notification_unsubscribe(
                    event->id, event->source_id, event->target_id);
            }
        }

        return FWK_SUCCESS;
    } else if (fwk_id_is_equal(
                   event->id, mod_scmi_notification_id_initialized)) {
        /* The subscribed SCMI channel has been initialized */
        scmi_notification_count++;
    } else if (fwk_id_is_equal(
                   event->id, mod_sds_notification_id_initialized)) {
        /* The SDS module has been initialized */
        sds_notification_received = true;
    } else {
        return FWK_E_PARAM;
    }

    if ((scmi_notification_count == FWK_ARRAY_SIZE(scmi_notification_table)) &&
        sds_notification_received) {
        messaging_stack_ready();

        scmi_notification_count = 0;
        sds_notification_received = false;
    }

    return FWK_SUCCESS;
}

const struct fwk_module module_scp_platform = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = MOD_SCP_PLATFORM_API_COUNT,
    .init = scp_platform_mod_init,
    .bind = scp_platform_bind,
    .process_bind_request = scp_platform_process_bind_request,
    .process_notification = scp_platform_process_notification,
    .start = scp_platform_start,
};

const struct fwk_module_config config_scp_platform = { 0 };
