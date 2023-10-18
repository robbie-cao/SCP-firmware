/*
 * Arm SCP/MCP Software
 * Copyright (c) 2022 - 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     MHU module device indexes.
 */

#ifndef SCP_MHU3_H
#define SCP_MHU3_H

enum kronos_mhu_devices {
    MHU3_DEVICE_IDX_LCP_AP_FCH_DVFS_SET_LVL,
    MHU3_DEVICE_IDX_LCP_AP_FCH_DVFS_SET_LIM_MIN,
    MHU3_DEVICE_IDX_LCP_AP_FCH_DVFS_SET_LIM_MAX,
    MHU3_DEVICE_IDX_COUNT,
};

#endif /* SCP_MHU3_H */
