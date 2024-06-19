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
# Function definitions for protobuf preprocessor and compiler
#
#

cmake_minimum_required(VERSION 3.20)

# Function to preprocess protobuf files
# Arguments: 
# . catena_interface_dir location of the protobuf interface definition files
# . proto_stems list of the names of the proto files to preprocess
# . model the runtime model to optimize for
#
# Returns:
#  ${model}_sources the list of filenames preprocessed
#
function(preprocess_protobuf_files catena_interface_dir proto_stems model)
    message(STATUS "catena_interface_dir: ${catena_interface_dir}") 
    message(STATUS "proto_stems: ${proto_stems}")
    message(STATUS "model: ${model}")
    set(optimize_for CODE_SIZE)
    if (${model} STREQUAL "lite")
        set(optimize_for LITE_RUNTIME)
    endif()
    message(STATUS "Building Catena model for ${model} runtime optimizing for ${optimize_for}")

    set(sources "${${model}_protos}")
    set(folder "${CMAKE_CURRENT_BINARY_DIR}/${model}")
    file(MAKE_DIRECTORY "${folder}")

    foreach(_proto ${proto_stems})
        # preprocess proto files
        set(input "${catena_interface_dir}${_proto}.proto")
        set(output "${folder}/${_proto}.proto")
        set(target "${_proto}_${model}")
        
        add_custom_command(
            OUTPUT "${output}"
            COMMAND ${CPP} ${cpp_opts} -D OPTIMIZE_FOR=${optimize_for} -o "${output}" "${input}"
            DEPENDS "${input}"
            COMMENT "Preprocessing ${input} into ${output}"
            VERBATIM
        )
        message(STATUS "Adding custom target ${target} for ${output}")
        add_custom_target(
            "${target}" ALL
            WORKING_DIRECTORY "${folder}"
            DEPENDS "${output}"
        )

        # keep a list of preprocessed protobuf source files
        list(APPEND sources "${output}")
    endforeach()
    set("${model}_protos" "${sources}" PARENT_SCOPE)
endfunction()

# Function to run the protobuf compiler on the preprocessed proto files
# build the list of generated source files into the sources variable
# Arguments:
function(generate_cpp_protobuf_sources catena_interface_dir proto_stems model gRPC)
    message(STATUS "catena_interface_dir: ${catena_interface_dir}") 
    message(STATUS "proto_stems: ${proto_stems}")
    message(STATUS "model: ${model}")

    message(STATUS "Building Catena cpp sources for ${model}")
    set(sources "${${model}_sources}")
    set(folder "${CMAKE_CURRENT_BINARY_DIR}/${model}")

    foreach(_proto ${proto_stems})
        set(input "${folder}/${_proto}.proto")
        set(output "${folder}/${_proto}.pb.h")
        set(target "${_proto}_${model}_code")

        # generate names of output files
        set(proto_cc "${_proto}.pb.cc")
        set(proto_h "${_proto}.pb.h")
        set(proto_grpc_cc "${_proto}.grpc.pb.cc")
        set(proto_grpc_h "${_proto}.grpc.pb.h")
        set(_sources "${folder}/${proto_cc}" "${folder}/${proto_h}")

        # set the arguments for the protobuf compiler
        set (protoc_args 
            "--cpp_out=${folder}"
            "-I"
            "${folder}"
            "${input}"
        )

        # add grpc generator to the service definition if gRPC is enabled
        if (${_proto} STREQUAL "service" AND ${gRPC} GREATER -1)
            list(APPEND protoc_args
            "--grpc_out=${folder}"
            "--plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}")
            message(STATUS "Adding gRPC generator for ${_proto}, transport ${CONNECTIONS}")
            set (_sources ${_sources} "${folder}/${proto_grpc_cc}" "${folder}/${proto_grpc_h}")
        endif()

        # accumulate list of all sources
        set(sources ${sources} ${_sources})

        # add custom command to generate the protobuf and gRPC sources
        add_custom_command(
            OUTPUT "${output}"
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS ${protoc_args}
            DEPENDS "${input}"
            COMMENT "Running C++ protocol buffer compiler on ${_proto}.proto"
            VERBATIM
        )

        # add the protobuf and grpc sources as targets
        add_custom_target(
            "${target}" ALL
            WORKING_DIRECTORY "${folder}"
            DEPENDS "${output}"
        )
    endforeach()
    set("${model}_sources" "${sources}" PARENT_SCOPE)
endfunction()