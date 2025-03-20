# Copyright 2024 Ross Video Ltd
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#



# Function to set up the proto targets
function(set_up_proto_targets)

    # Find Protobuf installation
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

    # set up our target, and build directory
    set(PROTO_TARGET "proto_interface") 
    set(PROTO_INTERFACE_DIR "${CMAKE_BINARY_DIR}/${PROTO_TARGET}")

    # The input to the protoc compiler is a set of preprocessed
    # .proto files. We need to make protoc's output depend on them
    add_custom_target(proto_preprocessor_target
        COMMAND ${CMAKE_COMMAND} -E echo "Added custom target proto_preprocessor_target"
    )

    # Process the proto files, produces list called proto_preprocessor_target_output
    preprocess_protobuf_files(
        "${catena_interface_dir}"
        "${interface_proto_stems}"
        "proto_preprocessor_target"
        "full"
        "${PROTO_INTERFACE_DIR}"
    )

    message(STATUS "proto_preprocessor_target_output: ${proto_preprocessor_target_output}")

    # declare our target
    # NB the sources are set in the call to preprocess_protobuf_files
    # Make protobuf_generate depend on preprocess_protobuf_files_target
    add_library(${PROTO_TARGET} STATIC)
    add_dependencies(${PROTO_TARGET} proto_preprocessor_target)
    target_compile_features(${PROTO_TARGET} PUBLIC cxx_std_20)

    # Specify location of output library
    set_target_properties(${PROTO_TARGET} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${PROTO_INTERFACE_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${PROTO_INTERFACE_DIR}/lib"
    )

    # Provide link libraries
    target_link_libraries(${PROTO_TARGET} PUBLIC protobuf::libprotobuf)

    # use the protobuf generate function provided by the protobuf package to make the c++ source files
    protobuf_generate(
        TARGET ${PROTO_TARGET}
        APPEND_PATH FALSE
        PROTOC_OUT_DIR ${PROTO_INTERFACE_DIR}/interface
        IMPORT_DIRS ${PROTO_INTERFACE_DIR}/preprocessed
        PROTOS ${proto_preprocessor_target_output}
        OUT_VAR cpp_sources
    )


    # Filter header files from cpp_sources
    set(header_files)
    foreach(_file ${cpp_sources})
        if(_file MATCHES "\\.h$")
            list(APPEND header_files ${_file})
        endif()
    endforeach()

    target_include_directories(
        ${PROTO_TARGET} PUBLIC
        $<BUILD_INTERFACE:${PROTOBUF_INCLUDE_DIRS}>
        # $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${PROTO_TARGET}>
    )

    # install the header files
    install(FILES ${header_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${PROTO_TARGET}/interface)

    # install the library
    install(TARGETS ${PROTO_TARGET}
        EXPORT Catena_cppTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    # put the target and its directory in parent scope
    set(PROTO_TARGET ${PROTO_TARGET} PARENT_SCOPE)
    set(PROTO_INTERFACE_DIR ${PROTO_INTERFACE_DIR} PARENT_SCOPE)

endfunction(set_up_proto_targets)