/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     DVFS handler module.
 */

#ifndef DVFS_HANDLER_H
#define DVFS_HANDLER_H

#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module_idx.h>

#include <stddef.h>
#include <stdint.h>

typedef struct dvfs_frame {
          uint32_t reserved0;
  FWK_R   uint32_t status;
          uint64_t reserved1;
  FWK_RW  uint32_t handshake_trigger;
          uint8_t  reserved2[0x20 - 0x14];
  FWK_RW  uint32_t policy_vlt;
          uint8_t  reserved3[0x30 - 0x24];
  FWK_R   uint32_t policy_vlt_st;
          uint32_t reserved4;
  FWK_R   uint32_t policy_vlt_st_raw;
          uint32_t reserved5;
  FWK_RW  uint32_t policy_freq;
          uint8_t  reserved6[0x50 - 0x44];
  FWK_R   uint32_t policy_freq_st;
          uint32_t reserved7;
  FWK_R   uint32_t policy_freq_st_raw;
          uint32_t reserved8;
  FWK_RW  uint32_t policy_ret_vlt;
          uint8_t  reserved9[0x70 - 0x64];
  FWK_R   uint32_t policy_ret_vlt_st;
          uint32_t reserved10;
  FWK_R   uint32_t policy_ret_vlt_st_raw;
          uint32_t reserved11;
  FWK_RW  uint32_t interrupt_st;
  FWK_RW  uint32_t interrupt_mask;
          uint8_t  reserved12[0xFC8 - 0x88];
  FWK_R   uint32_t impl_id;
          uint32_t reserved13;
  FWK_R   uint32_t arch_id;
} dvfs_frame_t;

/*!
 * \brief Performance limits.
 */
struct mod_dvfs_handler_perf_limits {
    uint32_t minimum; /*!< Minimum permitted level */
    uint32_t maximum; /*!< Maximum permitted level */
};

/*!
 * \brief Operating Performance Point (OPP).
 */
struct mod_dvfs_handler_opp {
    uint32_t level; /*!< Level value of the OPP. Cannot be 0 */
    uint32_t voltage; /*!< Power supply voltage in microvolts (uV) */
    uint32_t frequency; /*!< Clock rate in Hertz (Hz) */
};

/*!
 * \brief LCP DVFS configuration.
 */
struct mod_dvfs_handler_config {
    /*! Sustained operating point index */
    uint32_t sustained_idx;

    /*! Base address of LCP DVFS hardware */
    uint32_t dvfs_handler_addr;

    /*! IRQ number for interrupt from DVFS hardware */
    uint32_t dvfs_handler_irq_num;

    /*!
     * \brief Operating points.
     *
     * \note The frequencies and levels of these operating points must be in
     *      ascending order.
     */
    struct mod_dvfs_handler_opp *opps;

#ifdef BUILD_HAS_FAST_CHANNELS
    /* Fastchannels */
    fwk_id_t dvfs_fch_set_level;
    fwk_id_t dvfs_fch_set_limit_min;
    fwk_id_t dvfs_fch_set_limit_max;
#endif
};

/*!
 * \brief Domain API.
 */
struct mod_dvfs_handler_api {
    /*!
     * \brief Get the current operating point of a domain.
     *
     * \param [out] opp Current operating point.
     */
    int (*get_current_opp)(fwk_id_t domain_id,
        const struct mod_dvfs_handler_opp *opp);

    /*!
     * \brief Get the sustained operating point of a domain.
     *
     * \param [in] domain_id Element identifier of the domain.
     * \param [out] opp Sustained operating point.
     */
    int (*get_sustained_opp)(fwk_id_t domain_id,
        const struct mod_dvfs_handler_opp *opp);

    /*!
     * \brief Get the number of operating points of a domain.
     *
     * \param [in] domain_id Element identifier of the domain.
     * \param [out] opp_count Number of operating points.
     */
    int (*get_opp_count)(fwk_id_t domain_id, size_t *opp_count);

    /*!
     * \brief Set the level of a domain.
     *
     * \param [in] domain_id Element identifier of the domain.
     * \param [in] level Requested level.
     */
    int (*set_level)(fwk_id_t domain_id, uint32_t level);

    /*!
     * \brief Set the level of a domain.
     *
     * \param [in] domain_id Element identifier of the domain.
     * \param [in] limits Requested limits.
     */
    int (*set_limit)(fwk_id_t domain_id,
        struct mod_dvfs_handler_perf_limits limits);
};

/*!
 * \brief API indices.
 */
enum mod_dvfs_handler_api_idx {
    MOD_DVFS_HANDLER_API_IDX, /*!< API index for mod_dvfs_api_id_dvfs() */
    MOD_DVFS_HANDLER_API_IDX_COUNT /*!< Number of defined APIs */
};

/*! Module API identifier */
static const fwk_id_t mod_dvfs_handler_api_id =
    FWK_ID_API_INIT(FWK_MODULE_IDX_DVFS_HANDLER, MOD_DVFS_HANDLER_API_IDX);

#endif /* DVFS_HANDLER_H */
