# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

# ROCm-backed hipFile

set(HIPFILE_SOURCES
    ../common/hipfile-common.cpp
    async.cpp
    backend/asyncop-fallback.cpp
    backend/fallback.cpp
    backend/fastpath.cpp
    batch/batch.cpp
    buffer.cpp
    configuration.cpp
    context.cpp
    environment.cpp
    file.cpp
    hip.cpp
    hipfile.cpp
    mountinfo.cpp
    state.cpp
    stream.cpp
    sys.cpp
)

# Will create hipfile_(static|shared)
ais_add_libraries(
    NAME     hipfile
    DETAIL   amd
    LIBS     mount
    SRCS     ${HIPFILE_SOURCES}
    LIBINCL  ${HIPFILE_INCLUDE_PATH}
    SYSINCLS ${HIPFILE_AMD_SOURCE_PATH}
)
