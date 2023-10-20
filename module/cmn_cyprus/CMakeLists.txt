#
# Arm SCP/MCP Software
# Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
add_library(${SCP_MODULE_TARGET} SCP_MODULE)

target_include_directories(${SCP_MODULE_TARGET}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_sources(${SCP_MODULE_TARGET}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/mod_cmn_cyprus.c"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cmn_cyprus_discovery.c"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cmn_cyprus_utils.c"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cmn_cyprus_hnsam.c"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cmn_cyprus_rnsam.c"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cmn_cyprus_ccg.c")

target_link_libraries(${SCP_MODULE_TARGET}
    PRIVATE module-clock module-system-info module-timer)
