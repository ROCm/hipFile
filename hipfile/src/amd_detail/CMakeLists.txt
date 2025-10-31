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
