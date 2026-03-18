# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

#-----------------------------------------------------------------------------
# Option to build the hipFile documentation
#
# TODO: Consider turning this into two options, one of which just requires
#       Doxygen and produces API docs, the other produces Sphinx/Breathe
#       output
#-----------------------------------------------------------------------------
option(AIS_BUILD_DOCS "Build the hipFile docs (requires Doxygen, Sphinx, and Breathe)" OFF)

if(AIS_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    # Verify Sphinx + rocm_docs are available
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import rocm_docs"
        RESULT_VARIABLE _rocm_docs_found
        OUTPUT_QUIET ERROR_QUIET
    )
    if(NOT _rocm_docs_found EQUAL 0)
        message(FATAL_ERROR "rocm_docs Python package not found. Install it with: pip install rocm-docs-core")
    endif()

    # Set Doxygen input (pasted into Doxyfile.in)
    set(AIS_DOXYFILE_INPUT "${HIPFILE_ROOT_PATH}/include")
    
    # Set the path to the documentation
    set(AIS_DOC_PATH "${CMAKE_CURRENT_BINARY_DIR}/docs")
    
    # Set the Doxyfile install location
    set(AIS_DOXYFILE "${AIS_DOC_PATH}/doxygen/Doxyfile")

    # Doxygen XML output dir (consumed by Breathe inside Sphinx)
    set(AIS_DOXYGEN_XML_DIR "${AIS_DOC_PATH}/doxygen/xml")

    # Create the Doxyfile from the input file
    configure_file("docs/doxygen/Doxyfile.in" ${AIS_DOXYFILE})

    # Sphinx HTML output dir
    set(AIS_SPHINX_BUILD_DIR "${AIS_DOC_PATH}/html")

    # Build docs: Doxygen first, then Sphinx (which pulls in rocm_docs via conf.py)
    add_custom_target(doc
        # Step 1: Run Doxygen to produce XML for Breathe
        COMMAND "${DOXYGEN_EXECUTABLE}" "${AIS_DOXYFILE}"
        # Step 2: Run Sphinx, injecting the Doxygen paths via environment
        COMMAND
            ${CMAKE_COMMAND} -E env
                "DOXYFILE_PATH=${AIS_DOXYFILE}"
                "DOXYGEN_ROOT=${AIS_DOC_PATH}/doxygen"
                "DOXYGEN_XML_DIR=${AIS_DOXYGEN_XML_DIR}"
            "${Python3_EXECUTABLE}" -m sphinx
                -b html
                -C
                -c "${CMAKE_CURRENT_SOURCE_DIR}/docs/sphinx"
                "${CMAKE_CURRENT_SOURCE_DIR}/docs"
                "${AIS_SPHINX_BUILD_DIR}"
        WORKING_DIRECTORY "${AIS_DOC_PATH}"
        COMMENT "Generating hipFile API documentation with Doxygen + Sphinx (rocm_docs)"
        VERBATIM
    )

    # Set the output directory
    #set(DOXYGEN_OUT ${AIS_DOC_PATH})

    # Configure the documentation build
    #add_custom_target("doc"
    #    COMMAND ${DOXYGEN_EXECUTABLE} ${AIS_DOXYFILE}
    #    WORKING_DIRECTORY ${AIS_DOC_PATH}
    #    COMMENT "Generating hipFile API documentation with Doxygen"
    #    VERBATIM
    #)

endif()
