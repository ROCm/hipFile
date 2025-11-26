# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

# ROCm-backed hipFile

set(HIPFILE_SOURCES
    hipfile.cpp
    hipfile-rocfile.cpp
    ../common/hipfile-common.cpp
)

# Will create hipfile_(static|shared)
ais_add_libraries(
    NAME    hipfile
    DETAIL  amd
    SRCS    ${HIPFILE_SOURCES}
    DEPS    rocfile
    LIBINCL ${HIPFILE_INCLUDE_PATH}
)

set(ROCFILE_SOURCES
    async.cpp
    backend/asyncop-fallback.cpp
    backend/fallback.cpp
    backend/fastpath.cpp
    batch/batch.cpp
    buffer.cpp
    context.cpp
    file.cpp
    hip.cpp
    mountinfo.cpp
    rocfile.cpp
    rocfile-private.cpp
    state.cpp
    stream.cpp
    sys.cpp
)

# Will create rocfile_(static|shared)
ais_add_libraries(
    NAME     rocfile
    DETAIL   amd
    SRCS     ${ROCFILE_SOURCES}
    LIBINCL  ${ROCFILE_INCLUDE_PATH}
    SYSINCLS ${ROCFILE_SOURCE_PATH}
    LIBS     mount
)
