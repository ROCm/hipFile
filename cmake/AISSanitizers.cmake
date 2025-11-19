# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

option(AIS_BUILD_SANITIZERS "Build with -fsanitize=address, leak, and undefined" OFF)
option(AIS_BUILD_THREAD_SANITIZERS "Build with -fsanitize=thread (not compatible with AIS_BUILD_SANITIZERS)" OFF)

if(AIS_BUILD_THREAD_SANITIZERS)
    message(WARNING "TSAN has known problems with higher levels of entropy, try using `sudo sysctl vm.mmap_rnd_bits=28` if you encounter errors concerning unexpected memory mappings.")
endif()

function(ais_add_sanitizers target)
    if(AIS_BUILD_SANITIZERS AND AIS_BUILD_THREAD_SANITIZERS)
        message(FATAL_ERROR "AIS_BUILD_SANITIZERS is not compatible with AIS_BUILD_THREAD_SANITIZERS")
    endif()

    if(AIS_BUILD_SANITIZERS)
        # TODO: Consider other sanitizer options
        target_compile_options(${target} PRIVATE -fsanitize=address)
        target_link_options(${target} PRIVATE -fsanitize=address)
        target_compile_options(${target} PRIVATE -fsanitize=leak)
        target_link_options(${target} PRIVATE -fsanitize=leak)
        target_compile_options(${target} PRIVATE -fsanitize=undefined)
        target_link_options(${target} PRIVATE -fsanitize=undefined)

        target_compile_options(${target} PRIVATE -fno-omit-frame-pointer)
    endif()

    if(AIS_BUILD_THREAD_SANITIZERS)
        target_compile_options(${target} PRIVATE -fsanitize=thread)
        target_link_options(${target} PRIVATE -fsanitize=thread)
    endif()
endfunction()
