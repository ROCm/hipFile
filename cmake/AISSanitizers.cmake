# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

option(AIS_USE_SANITIZERS "Build with -fsanitize=address, leak, and undefined" OFF)
option(AIS_USE_THREAD_SANITIZER "Build with -fsanitize=thread (not compatible with AIS_USE_SANITIZERS)" OFF)
option(AIS_USE_INTEGER_SANITIZER "Build with -fsanitize=integer (clang only)" OFF)

if(AIS_USE_THREAD_SANITIZER)
    message(WARNING "TSAN has known problems with higher levels of entropy, try using `sudo sysctl vm.mmap_rnd_bits=28` if you encounter errors concerning unexpected memory mappings.")
endif()

function(ais_add_sanitizers target)
    if(AIS_USE_SANITIZERS AND AIS_USE_THREAD_SANITIZER)
        message(FATAL_ERROR "AIS_USE_SANITIZERS is not compatible with AIS_USE_THREAD_SANITIZER")
    endif()

    if(AIS_USE_SANITIZERS)
        # TODO: Consider other sanitizer options
        target_compile_options(${target} PRIVATE -fsanitize=address)
        target_link_options(${target} PRIVATE -fsanitize=address)
        target_compile_options(${target} PRIVATE -fsanitize=leak)
        target_link_options(${target} PRIVATE -fsanitize=leak)
        target_compile_options(${target} PRIVATE -fsanitize=undefined)
        target_link_options(${target} PRIVATE -fsanitize=undefined)

        target_compile_options(${target} PRIVATE -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fuse-ld=lld)
    endif()

    if(AIS_USE_THREAD_SANITIZER)
        target_compile_options(${target} PRIVATE -fsanitize=thread)
        target_link_options(${target} PRIVATE -fsanitize=thread)
    endif()

    # clang-only
    #
    # This has a very small runtime penalty (<1%) and will kill
    # the process if undefined integer behaviour occurs (like
    # wrapping a signed integer).
    if(AIS_USE_INTEGER_SANITIZER)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:CXX>:-fsanitize=integer,undefined>
                $<$<COMPILE_LANGUAGE:CXX>:-fno-sanitize-recover=integer>
            )
            target_link_options(${target} PRIVATE
                -Xarch_host -fsanitize=integer,undefined
                -Xarch_host -fno-sanitize-recover=integer
                -fuse-ld=lld
                --rtlib=compiler-rt
                --unwindlib=libgcc
            )
        else()
            message(FATAL_ERROR "The integer sanitizer is only available on clang")
        endif()
    endif()

endfunction()
