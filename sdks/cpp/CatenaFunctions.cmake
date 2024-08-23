#
# Licensed under the Creative Commons Attribution NoDerivatives 4.0 International Licensing (CC-BY-ND-4.0);
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
#  https://creativecommons.org/licenses/by-nd/4.0/
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Function definition for protobuf preprocessor
#
#

cmake_minimum_required(VERSION 3.20)

# Function to preprocess protobuf files
# Arguments: 
# . catena_interface_dir location of the protobuf interface definition files
# . proto_stems list of the names of the proto files to preprocess
# . proto_target the target that the preprocessed files will be added to
# . model the runtime model to optimize for
#
# Returns:
#  ${model}_sources the list of filenames preprocessed
#
function(preprocess_protobuf_files catena_interface_dir proto_stems proto_target model)
    
    set(optimize_for CODE_SIZE)
    if (${model} STREQUAL "lite")
        set(optimize_for LITE_RUNTIME)
    endif()
    message(STATUS "Building Catena model for ${model} runtime optimizing for ${optimize_for}")

    foreach(_proto ${proto_stems})
        # preprocess proto files
        set(input "${catena_interface_dir}${_proto}.proto")
        set(output "${CMAKE_BINARY_DIR}/${model}/${_proto}.proto")
        
        add_custom_command(
            OUTPUT "${output}"
            COMMAND ${CPP} ${cpp_opts} -D OPTIMIZE_FOR=${optimize_for} -o "${output}" "${input}"
            DEPENDS "${input}"
            COMMENT "Preprocessing ${input} into ${output}"
            VERBATIM
        )

        set_source_files_properties(${output} PROPERTIES GENERATED TRUE)
        target_sources(${proto_target} PRIVATE ${output})

    endforeach()
endfunction()

# Function to install Catena codegen
function(install_catena_codegen)

    #install the Catena device validation schema file
    install(
        FILES ${CATENA_ROOT_DIR}/schema/catena.schema.json
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/Catena_cpp
    )
    set(CATENA_SCHEMA_JSON ${CMAKE_INSTALL_DATAROOTDIR}/Catena_cpp/catena.schema.json PARENT_SCOPE)

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
endfunction()

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
        # Run 'make doxygen' in the output directory to build the target.
        add_custom_target(doxygen
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating documentation with Doxygen"
            VERBATIM
        )


    elseif(BUILD_DOC)
        message("Doxygen needs to be installed to generate the documentation")
    endif (DOXYGEN_FOUND AND BUILD_DOC)
endfunction()
