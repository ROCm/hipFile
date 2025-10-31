# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

#-----------------------------------------------------------------------------
# Warn about unsafe buffer operations (llvm C++ only)
#
# See: https://clang.llvm.org/docs/SafeBuffers.html for more info
#
# Fixing this cleanly requires C++20, so it's an option for now
#-----------------------------------------------------------------------------
option(AIS_WARN_UNSAFE_BUFFER_OPS "Warn about unsafe buffer operations (llvm C++ only)" OFF)
if(AIS_WARN_UNSAFE_BUFFER_OPS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(AIS_CLANG_WARNING_FLAGS
            -Wunsafe-buffer-usage
            -fsafe-buffer-usage-suggestions
            ${AIS_CLANG_WARNING_FLAGS}
        )
    endif()
endif()
