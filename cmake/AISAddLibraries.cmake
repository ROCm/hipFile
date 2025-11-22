# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

include(AISCompilerOptions)
include(CheckLinkerFlag)

# Add static and shared libraries using AIS build conventions
#
# Parameters:
#   NAME     <name>                             The name of the libraries to create
#   DETAIL   "amd"|"nvidia"                     Whether this is an AMD or NVIDIA target
#   LIBINCL  <path>                             Path to installed include files (e.g., hipfile.h)
#   SYSINCLS [path1 [path2 ...]]                Paths to other include dirs
#   SRCS     [src1 [src2 ...]]                  The source files
#   DEPS     [dependency1 [dependency2 ...]]    List of AIS target library dependencies
#   LIBS     [lib1, [lib2 ...]]                 List of external library dependencies
#
# NOTE: Assumes internal library dependencies are named <foo>_(static|shared)
#
# NOTE: This isn't the most robust function. It's mainly intended to reduce
#       code duplication in rocFile/hipFile. For example, DEPS is for passing
#       the rocFile internal library dependency to hipFile and won't work for
#       passing general library dependencies.
function(ais_add_libraries)

    # Parse arguments
    set(options) # None at this time
    set(oneValueArgs NAME DETAIL LIBINCL)
    set(multiValueArgs SRCS DEPS SYSINCLS LIBS)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Convenience names
    set(object_name "${arg_NAME}_object")
    set(static_name "${arg_NAME}_static")
    set(shared_name "${arg_NAME}_shared")

    # Add the object, shared, and static library targets
    #
    # The static and shared libraries are both built from the
    # same compiled objects (with -fPIC, etc.)
    add_library(${object_name} OBJECT ${arg_SRCS})
    add_library(${static_name} STATIC $<TARGET_OBJECTS:${object_name}>)
    add_library(${shared_name} SHARED $<TARGET_OBJECTS:${object_name}>)

    # Add version numbers
    set_target_properties(${static_name} PROPERTIES VERSION ${AIS_LIBRARY_VERSION})
    set_target_properties(${shared_name} PROPERTIES VERSION ${AIS_LIBRARY_VERSION})
    set_target_properties(${shared_name} PROPERTIES SOVERSION ${AIS_LIBRARY_SOVERSION})

    # Add dependencies on internal targets
    foreach(dep IN LISTS arg_DEPS)
        add_dependencies(${static_name} "${dep}_static")
        add_dependencies(${shared_name} "${dep}_shared")

        target_link_libraries(${static_name} PRIVATE "${dep}_static")
        target_link_libraries(${shared_name} PRIVATE "${dep}_shared")

        # The object library needs to see the headers for any dependencies
        target_link_libraries(${object_name} PUBLIC "${dep}_shared")
    endforeach()

    # Add dependencies on external libraries
    foreach(lib IN LISTS arg_LIBS)
        find_library(LIBRARY_PATH NAMES ${lib})
        if(NOT LIBRARY_PATH)
            message(FATAL_ERROR "lib${lib} not found")
        endif()
        target_link_libraries(${static_name} PRIVATE ${LIBRARY_PATH})
        target_link_libraries(${shared_name} PRIVATE ${LIBRARY_PATH})
    endforeach()

    foreach(target "${object_name}" "${static_name}" "${shared_name}")

        if(BUILD_TESTING)
            target_compile_definitions(${target} PRIVATE AIS_TESTING)
        endif()

        if(arg_DETAIL STREQUAL "nvidia")
            target_compile_definitions(${target} PRIVATE __HIP_PLATFORM_NVIDIA__)
        else()
            target_compile_definitions(${target} PRIVATE __HIP_PLATFORM_AMD__)
        endif()

        # Set the library name, minus the .a, .so, or whatever
        set_target_properties(${target} PROPERTIES OUTPUT_NAME ${arg_NAME})

        # Set the public include paths (these are installed libraries)
        target_include_directories(${target}
            PUBLIC
            $<BUILD_INTERFACE:${arg_LIBINCL}>
            $<INSTALL_INTERFACE:include>
        )

        # Add other include paths
        foreach(incl IN LISTS arg_SYSINCLS)
            target_include_directories(${target} SYSTEM PRIVATE ${incl})
        endforeach()

        # Add libraries and headers for NVIDIA targets
        if(arg_DETAIL STREQUAL "nvidia")
            target_include_directories(${target}
                SYSTEM
                PRIVATE
                ${HIP_INCLUDE_DIRS}
                ${CUDAToolkit_INCLUDE_DIRS}
            )
            target_link_libraries(${target} PRIVATE ${CUFILE_LIBRARY})
        endif()
        target_link_libraries(${target} PRIVATE hip::host)

        # Add link options (mainly for hardening)
        check_linker_flag(CXX "-Wl,-z,noexecstack" LINKER_SUPPORTS_NOEXECSTACK)
        if(LINKER_SUPPORTS_NOEXECSTACK)
            target_link_options(${target} PRIVATE "-Wl,-z,noexecstack")
        endif()

        # Add the common include path
        target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/shared")

        # Set compiler flags
        ais_set_compiler_flags(${target})
    endforeach()
endfunction()
