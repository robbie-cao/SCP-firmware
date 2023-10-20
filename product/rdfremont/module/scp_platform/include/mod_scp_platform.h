/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SCP Platform Support
 */

#ifndef MOD_SCP_PLATFORM_H
#define MOD_SCP_PLATFORM_H

#include <mod_power_domain.h>

/*!
 * \addtogroup GroupPLATFORMModule PLATFORM Product Modules
 * @{
 */

/*!
 * \defgroup GroupSCPPlatform SCP Platform Support
 * @{
 */

/*!
 * \brief SCP Platform power states.
 */
enum mod_scp_platform_power_states {
    MOD_SCP_PLATFORM_POWER_STATE_SLEEP0 = MOD_PD_STATE_COUNT,
    MOD_SCP_PLATFORM_POWER_STATE_SLEEP1,
    MOD_SCP_PLATFORM_POWER_STATE_COUNT
};

/*!
 * \brief System power state masks.
 */
enum mod_scp_platform_power_state_masks {
    MOD_SCP_PLATFORM_POWER_STATE_SLEEP0_MASK =
        (1 << MOD_SCP_PLATFORM_POWER_STATE_SLEEP0),
    MOD_SCP_PLATFORM_POWER_STATE_SLEEP1_MASK =
        (1 << MOD_SCP_PLATFORM_POWER_STATE_SLEEP1),
};

/*!
 * \brief Indices of the interfaces exposed by the module.
 */
enum mod_scp_platform_api_idx {
    /*! API index for the driver interface of the SYSTEM POWER module */
    MOD_SCP_PLATFORM_API_IDX_SYSTEM_POWER_DRIVER,

    /*! Interface for Transport module */
    MOD_SCP_PLATFORM_API_IDX_TRANSPORT_SIGNAL,

    /*! Number of exposed interfaces */
    MOD_SCP_PLATFORM_API_COUNT
};

/*!
 * @}
 */

/*!
 * @}
 */

#endif /* MOD_SCP_PLATFORM_H */
