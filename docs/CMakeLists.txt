# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

# Documentation CMake file for AIS

find_package(Doxygen REQUIRED)

# The top-level doc target
add_custom_target(doc COMMENT "Building AIS documentation")

# Add a target of the name AIS_DOC_PRETTY_NAME-doc and make
# it a dependency of the top-level doc target
#
# NOTE: The Doxyfile used in this function is optimized
#       for C output and unsuitable for internal C++
#       documentation
function(ais_add_docs_target AIS_DOC_PRETTY_NAME)
    # Also need the lower-case name for files, etc.
    string(TOLOWER ${AIS_DOC_PRETTY_NAME} AIS_DOC_LOWER_NAME)

    # Set Doxyfile variables
    set(AIS_DOXYFILE_PROJECT_NAME  "${AIS_DOC_PRETTY_NAME}")
    set(AIS_DOXYFILE_PROJECT_BRIEF "${AIS_DOC_PRETTY_NAME} API documentation")
    set(AIS_DOXYFILE_INPUT         "${CMAKE_SOURCE_DIR}/${AIS_DOC_LOWER_NAME}/include")

    # Set the path to the documentation
    set(AIS_DOC_PATH "${CMAKE_CURRENT_BINARY_DIR}/${AIS_DOC_LOWER_NAME}")

    # Set that target name
    set(AIS_DOC_TARGET_NAME "${AIS_DOC_LOWER_NAME}-doc")

    # Set the Doxyfile install location
    set(AIS_DOXYFILE ${AIS_DOC_PATH}/Doxyfile)

    # Create the Doxyfile from the input file
    configure_file(Doxyfile.in ${AIS_DOXYFILE})

    # Set the output directory
    set(DOXYGEN_OUT ${AIS_DOC_PATH})

    # Configure the documentation build
    add_custom_target(${AIS_DOC_TARGET_NAME}
        COMMAND ${DOXYGEN_EXECUTABLE} ${AIS_DOXYFILE}
        WORKING_DIRECTORY ${AIS_DOC_PATH}
        COMMENT "Generating ${AIS_DOC_PRETTY_NAME} API documentation with Doxygen"
        VERBATIM
    )

    # Add a dependency to the top-level doc project
    add_dependencies(doc ${AIS_DOC_TARGET_NAME})
endfunction()

ais_add_docs_target("hipFile")
if(BUILD_ROCFILE)
    ais_add_docs_target("rocFile")
endif()
