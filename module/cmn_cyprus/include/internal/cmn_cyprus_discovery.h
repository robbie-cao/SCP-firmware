/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions and utility functions for the CMN-Cyprus module.
 */

#ifndef INTERNAL_CMN_CYPRUS_DISCOVERY_H
#define INTERNAL_CMN_CYPRUS_DISCOVERY_H

/*
 * Discover the topology of the interconnect and setup the context data.
 *
 * \param ctx Pointer to the driver context.
 *
 * \return Nothing.
 */
void cmn_cyprus_discovery(struct cmn_cyprus_ctx *ctx);

#endif /* INTERNAL_CMN_CYPRUS_DISCOVERY_H */
