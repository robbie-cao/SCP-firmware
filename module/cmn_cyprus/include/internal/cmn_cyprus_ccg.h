/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions and utility functions for programming CCG.
 */

#ifndef INTERNAL_CMN_CYPRUS_CCG_H
#define INTERNAL_CMN_CYPRUS_CCG_H

#include <internal/cmn_cyprus_ctx.h>

/*
 * Program the CML and enable SMP.
 *
 * \param ctx Pointer to the driver context.
 *
 * \return Nothing.
 */
void cmn_cyprus_setup_cml(struct cmn_cyprus_ctx *ctx);

#endif /* INTERNAL_CMN_CYPRUS_CCG_H */
