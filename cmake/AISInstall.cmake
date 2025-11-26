# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

# Install hipFile and rocFile

# From the rocm-cmake repo
include(ROCMInstallTargets)
include(ROCMCreatePackage)

# Install the package
if(BUILD_AMD_DETAIL)
    rocm_install(TARGETS rocfile_static)
    rocm_install(TARGETS rocfile_shared)
endif()
rocm_install(TARGETS hipfile_static)
rocm_install(TARGETS hipfile_shared)

# Install the headers
rocm_install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/hipfile/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Always install the hipfile-types.h file
rocm_install(
    FILES ${CMAKE_SOURCE_DIR}/shared/hipfile-types.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# rocm_package_add_dependencies() line should go here
# when we have dependencies

# Export the targets
if(BUILD_AMD_DETAIL)
    set(target_list ${target_list} roc::rocfile_shared roc::rocfile_static)
endif()
set(target_list ${target_list} roc::hipfile_shared roc::hipfile_static)

rocm_export_targets(
    TARGETS ${target_list}
    NAMESPACE roc::
)

# CPack license setup
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.md")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")

# Create the package
rocm_create_package(
    NAME "hipFile"
    DESCRIPTION "The hipFile library"
    MAINTAINER "ais-eap <ais-eap@amd.com>"
)
