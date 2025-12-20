# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

# Install hipFile

# From the rocm-cmake repo
include(ROCMInstallTargets)
include(ROCMCreatePackage)

# Install the package
rocm_install(TARGETS hipfile)

# Install the headers
rocm_install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# When we have RELEASE/DEV builds set up, we can split
# where these dependencies are added.
# For now, just add both runtime & dev deps.

# AMD Runtime Dependencies
if(CMAKE_HIP_PLATFORM STREQUAL "amd")
    rocm_package_add_dependencies(DEPENDS hip-runtime-amd) # Need minimum version
    # Suppressing libmount RELEASE dependency for now for the following:
    # 1) We currently default to DEV builds.
    # 2) libmount naming convention is not consistent across distros.
    #    We need to figure out the ideal method for handling this case.
    #    openSUSE (RPM): libmount1
    #    rocky    (RPM): libmount
    #    Ubuntu   (DEB): libmount1
    #rocm_package_add_dependencies(DEPENDS libmount)
endif()

# AMD Development Dependencies
if(CMAKE_HIP_PLATFORM STREQUAL "amd")
    rocm_package_add_deb_dependencies(DEPENDS hip-dev) # Need minimum version
    rocm_package_add_rpm_dependencies(DEPENDS hip-devel)

    rocm_package_add_deb_dependencies(DEPENDS libmount-dev)
    rocm_package_add_rpm_dependencies(DEPENDS libmount-devel)
endif()

# Nvidia Runtime Dependencies
if(CMAKE_HIP_PLATFORM STREQUAL "nvidida")
    rocm_package_add_dependencies(DEPENDS libcufile)
endif()

# Nvidia Development Dependencies
if(CMAKE_HIP_PLATFORM STREQUAL "nvidia")
    rocm_package_add_deb_dependencies(DEPENDS libcufile-dev)
    rocm_package_add_rpm_dependencies(DEPENDS libcufile-devel)
endif()

# Export the targets
set(target_list ${target_list} roc::hipfile)

rocm_export_targets(
    TARGETS ${target_list}
    NAMESPACE roc::
)

# CPack license setup
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.md")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")

# rocm-cmake sets CPACK_SET_DESTDIR on Linux, which conflicts with
# asking for relocatable packages
set(CPACK_PACKAGE_RELOCATABLE OFF)
set(CPACK_RPM_PACKAGE_RELOCATABLE OFF)
set(CPACK_DEB_PACKAGE_RELOCATABLE OFF)

# Create the package
#
# Note that you will just get a dev package if you have BUILD_SHARED_LIBS
# set to off (since there are no shared libraries to install).
rocm_create_package(
    NAME "hipFile"
    DESCRIPTION "The hipFile library"
    MAINTAINER "hipfile-maintainer@amd.com"
)
