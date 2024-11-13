# Copyright 2024 Ross Video Ltd
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
# Function definition for protobuf preprocessor
#
#

cmake_minimum_required(VERSION 3.20)

# Function to preprocess protobuf files
# Arguments: 
# . catena_interface_dir location of the protobuf interface definition files
# . proto_stems list of the names of the proto files to preprocess
# . target the target that the preprocessed files will be added to
# . model the runtime model to optimize for
#
# Returns:
#  ${model}_sources the list of filenames preprocessed
#
function(preprocess_protobuf_files catena_interface_dir proto_stems target model out_dir)
    
    set(optimize_for SPEED)
    if (${model} STREQUAL "lite")
        set(optimize_for LITE_RUNTIME)
    endif()
    message(STATUS "Building Catena model for ${model} runtime optimizing for ${optimize_for}")
    set(preprocessed_files)
    foreach(_stem ${proto_stems})
        # preprocess proto files
        set(input "${catena_interface_dir}${_stem}.proto")
        set(output "${out_dir}/preprocessed/${_stem}.proto")
        
        add_custom_command(
            OUTPUT "${output}"
            COMMAND ${CPP} ${cpp_opts} -D SPEED=${optimize_for} -o "${output}" "${input}"
            DEPENDS "${input}"
            COMMENT "Preprocessing ${input} into ${output}"
            VERBATIM
        )

        set_source_files_properties(${output} PROPERTIES GENERATED TRUE)
        list(APPEND preprocessed_files "${output}")
    endforeach()
    target_sources(${target} PRIVATE ${preprocessed_files})
    set(${target}_output ${preprocessed_files} PARENT_SCOPE)
endfunction(preprocess_protobuf_files)

# Function to install Catena codegen
function(install_catena_codegen)

    #install the Catena device validation schema file
    install(
        FILES ${CATENA_ROOT_DIR}/schema/catena.device_schema.json
            ${CATENA_ROOT_DIR}/schema/catena.param_schema.json
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/Catena_cpp
    )
    set(CATENA_SCHEMA_JSON ${CMAKE_INSTALL_DATAROOTDIR}/Catena_cpp PARENT_SCOPE)

    # Ensure the custom target runs during the installation process
    find_program(NPM_EXECUTABLE npm REQUIRED)
    get_filename_component(codegen_dir ${CMAKE_SOURCE_DIR}/../../tools/codegen ABSOLUTE)
    get_filename_component(install_dir ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/Catena_cpp ABSOLUTE)
    install(CODE 
        "message(STATUS \"Installing Catena codegen\")
        execute_process(
            COMMAND ${NPM_EXECUTABLE} install ${codegen_dir} --install-links
            WORKING_DIRECTORY ${install_dir}
        )"
    )
    set(CATENA_CODEGEN_INSTALL_DIR ${CMAKE_INSTALL_DATAROOTDIR}/Catena_cpp/node_modules/catena-codegen PARENT_SCOPE)
    
    install(FILES ${CMAKE_SOURCE_DIR}/CatenaCodegen.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Catena_cpp)
endfunction(install_catena_codegen)

# Function to build the documentation for Catena
function(build_docs)
    # doxygen
    #
    option(BUILD_DOC "Build documentation" ON)

    # check if Doxygen is installed
    find_package(Doxygen OPTIONAL_COMPONENTS dot)

    if (DOXYGEN_FOUND AND BUILD_DOC)
        # set input and output files
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/doxyfile)
        set(COMMON_EXAMPLES ${CMAKE_CURRENT_SOURCE_DIR}/common/examples)
        set(gRPC_EXAMPLES ${CMAKE_CURRENT_SOURCE_DIR}/connections/grpc/examples)
        set(REST_EXAMPLES ${CMAKE_CURRENT_SOURCE_DIR}/connections/rest/examples)
        set(DOXYGEN_EXAMPLES_PATHS "${COMMON_EXAMPLES} ${gRPC_EXAMPLES} ${REST_EXAMPLES}")
        set(VDK_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/common/include/vdk)
        set(VDK_SRC ${CMAKE_CURRENT_SOURCE_DIR}/common/src/vdk)
        set(DOXYGEN_EXCLUDE "${VDK_INCLUDE} ${VDK_SRC}")

        # find out if graphviz is installed
        if (DOXYGEN_DOT_EXECUTABLE STREQUAL "DOXYGEN_DOT_EXECUTABLE-NOTFOUND")
            # it isn't, so print a warning and suppress DOT in the doxyfile

            message(AUTHOR_WARNING "graphviz needs to be installed for the documentation to include \
            enhanced inheritance and collaboration diagrams.\
            Otherwise, it'll only generate standard doxygen inheritance diagrams.\
            See https://www.graphviz.org/download/ for how to install graphviz."
            )
            set(DOXYGEN_HAVE_DOT NO)

        else ()
            # graphviz is installed, so enable DOT in the doxyfile
            set(DOXYGEN_HAVE_DOT YES)

        endif ()

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        # Creates a target 'doxygen' to generate the documentation.
        # If DOCS_ONLY is set, the target is added to ALL so that it will build
        # from a simple invocation of the generator - e.g. $ ninja.
        # Otherswise, it isn't added to ALL because we're more than likely
        # in a code-test-fix workflow and don't want to waste time generating
        # documentation.
        set(DOXYGEN_IS_IN_ALL)
        if(ONLY_DOCS)
            set(DOXYGEN_IS_IN_ALL ALL)
        endif(ONLY_DOCS)
        add_custom_target(doxygen ${DOXYGEN_IS_IN_ALL}
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating documentation with Doxygen"
            VERBATIM
        )
    elseif(BUILD_DOC)
        message("Doxygen needs to be installed to generate the documentation")
    endif (DOXYGEN_FOUND AND BUILD_DOC)
endfunction(build_docs)
