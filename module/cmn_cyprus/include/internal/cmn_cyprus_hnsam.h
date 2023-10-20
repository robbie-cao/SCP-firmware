/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions and utility functions for the programming HN-SAM.
 */

#ifndef INTERNAL_CMN_CYPRUS_HNSAM_H
#define INTERNAL_CMN_CYPRUS_HNSAM_H

#include <internal/cmn_cyprus_ctx.h>

/*
 * Program the HN-F SAM.
 *
 * \param ctx Pointer to the driver context.
 *
 * \return Nothing.
 */
void cmn_cyprus_setup_hnf_sam(struct cmn_cyprus_ctx *ctx);

#endif /* INTERNAL_CMN_CYPRUS_HNSAM_H */
