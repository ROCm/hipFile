# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

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
