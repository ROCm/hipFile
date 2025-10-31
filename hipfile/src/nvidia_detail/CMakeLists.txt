# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

# CUDA-backed hipFile

set(HIPFILE_SOURCES
    hipfile.cpp
    hipfile-cufile.cpp
    ../common/hipfile-common.cpp
)

# Will create hipfile_(static|shared)
ais_add_libraries(
    NAME    hipfile
    DETAIL  nvidia
    SRCS    ${HIPFILE_SOURCES}
    LIBINCL ${HIPFILE_INCLUDE_PATH}
)
