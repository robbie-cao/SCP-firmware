/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions and utility functions for the programming CMN-Cyprus RN SAM.
 */

#ifndef INTERNAL_CMN_CYPRUS_RNSAM_H
#define INTERNAL_CMN_CYPRUS_RNSAM_H

#include <internal/cmn_cyprus_ctx.h>

/*
 * Program the RN SAM.
 *
 * \param ctx Pointer to the driver context.
 *
 * \return Nothing.
 */
void cmn_cyprus_setup_rnsam(struct cmn_cyprus_ctx *ctx);

/*
 * Get RN SAM Memmap API.
 *
 * \param api Pointer to the API pointer variable.
 *
 * \return Nothing.
 */
void get_rnsam_memmap_api(const void **api);

#endif /* INTERNAL_CMN_CYPRUS_RNSAM_H */
