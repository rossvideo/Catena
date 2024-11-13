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



# Function to set up the gRPC targets
function(set_up_gRPC_targets)

    # Find Protobuf installation
    if(CMAKE_CROSSCOMPILING)
        # Change protobuf::protoc to reference the protoc executable found on the host system
        find_program(_PROTOBUF_PROTOC protoc REQUIRED)
        get_target_property(protoc_config protobuf::protoc IMPORTED_CONFIGURATIONS)
        set_target_properties(protobuf::protoc PROPERTIES IMPORTED_LOCATION_${protoc_config} ${_PROTOBUF_PROTOC})
        get_target_property(imported_location protobuf::protoc IMPORTED_LOCATION_${protoc_config})
        message(STATUS "Using protoc compiler: ${imported_location}")
    else()
        set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
    endif()

    # Find gRPC installation
    # Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
    find_package(gRPC CONFIG REQUIRED)
    message(STATUS "Using gRPC ${gRPC_VERSION}")

    # choose correct grpc_cpp_plugin
    if(CMAKE_CROSSCOMPILING)
        find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
    else()
        set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
    endif()

    # set up our target, and build directory
    set(GRPC_TARGET "grpc_service") 
    set(GRPC_SERVICE_DIR "${CMAKE_BINARY_DIR}/${GRPC_TARGET}")

    # add service to the list of protobuf file stems
    set(grpc_proto_stems ${interface_proto_stems})
    list(APPEND grpc_proto_stems "service")

    # The input to the protoc compiler is a set of preprocessed
    # .proto files. We need to make protoc's output depend on them
    add_custom_target(grpc_preprocessor_target
        COMMAND ${CMAKE_COMMAND} -E echo "Added custom target grpc_preprocessor_target"
    )

    # Process the proto files, produces list called grpc_preprocessor_target_output
    preprocess_protobuf_files(
        "${catena_interface_dir}"
        "${grpc_proto_stems}"
        "grpc_preprocessor_target"
        "lite"
        "${GRPC_SERVICE_DIR}"
    )

    message(STATUS "grpc_preprocessor_target_output: ${grpc_preprocessor_target_output}")

    # gRPC needs a couple of extra files that need to be flagged as interface and required
    # by the grpc_service target
    set(extra_files
        "${GRPC_SERVICE_DIR}/interface/service.grpc.pb.h"
        "${GRPC_SERVICE_DIR}/interface/service.grpc.pb.cc"
    )
    set_source_files_properties(${extra_files} PROPERTIES GENERATED TRUE)

    # declare our target
    # NB the sources are set in the call to preprocess_protobuf_files
    # apart from the extra files
    # Make protobuf_generate depend on preprocess_protobuf_files_target
    add_library(${GRPC_TARGET} STATIC)
    target_sources(${GRPC_TARGET} PRIVATE ${extra_files})
    add_dependencies(${GRPC_TARGET} grpc_preprocessor_target)
    target_compile_features(${GRPC_TARGET} PUBLIC cxx_std_20)

    # Specify location of output library
    set_target_properties(${GRPC_TARGET} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${GRPC_SERVICE_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${GRPC_SERVICE_DIR}/lib"
    )

    # Provide link libraries
    target_link_libraries(${GRPC_TARGET} PUBLIC protobuf::libprotobuf-lite gRPC::grpc++)

    # use the protobuf generate function provided by the protobuf package to make the c++ source files
    protobuf_generate(
        TARGET ${GRPC_TARGET}
        APPEND_PATH FALSE
        PROTOC_OUT_DIR ${GRPC_SERVICE_DIR}/interface
        IMPORT_DIRS ${GRPC_SERVICE_DIR}/preprocessed
        PROTOS ${grpc_preprocessor_target_output}
        OUT_VAR cpp_sources
    )

    # generate the extra files for the grpc_service target
    protobuf_generate(
        PROTOS ${GRPC_SERVICE_DIR}/preprocessed/service.proto
        LANGUAGE grpc
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        APPEND_PATH FALSE
        PROTOC_OUT_DIR ${GRPC_SERVICE_DIR}/interface
        PLUGIN "protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        OUT_VAR output
    )

    # Filter header files from cpp_sources
    set(header_files)
    foreach(_file ${cpp_sources})
        if(_file MATCHES "\\.h$")
            list(APPEND header_files ${_file})
        endif()
    endforeach()

    target_include_directories(
        ${GRPC_TARGET} PUBLIC
        # $<BUILD_INTERFACE:${PROTOBUF_INCLUDE_DIRS}>
        # $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${GRPC_TARGET}>
    )

    # install the header files
    install(FILES ${header_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${GRPC_TARGET})

    # install the library
    install(TARGETS ${proto_interface}
        EXPORT Catena_cppTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    # put the target and its directory in parent scope
    set(GRPC_TARGET ${GRPC_TARGET} PARENT_SCOPE)
    set(GRPC_SERVICE_DIR ${GRPC_SERVICE_DIR} PARENT_SCOPE)

endfunction(set_up_gRPC_targets)