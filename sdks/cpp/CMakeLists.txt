# Copyright © 2023 Ross Video Ltd
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
# cmake build file for C++ SDKs and examples.
#
# Assumes protobuf and gRPC have been installed using cmake.
# See here for the quickstart guide to do this:
#
#  https://grpc.io/docs/languages/cpp/quickstart/
#
#

cmake_minimum_required(VERSION 3.8)

project(CPP_SDKs C CXX)

# Assumption: gRPC and all its dependencies are already installed
# on this system, so they can be located by find_package().

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
set(GRPC_LIB_CMAKE "$ENV{HOME}/.local/lib/cmake")
set(CMAKE_PREFIX_PATH "${GRPC_LIB_CMAKE}/protobuf;${GRPC_LIB_CMAKE}/grpc;${GRPC_LIB_CMAKE}/grpc/modules;${GRPC_LIB_CMAKE}/absl")

set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
find_program(_PROTOBUF_PROTOC protoc)
else()
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

# Identify Catena proto files
#
get_filename_component(catena_service "../../interface/service.proto" ABSOLUTE)
get_filename_component(catena_proto_path "${catena_service}" PATH)
set(lang_proto ${catena_proto_path}/language.proto)

# Generated sources
set(lang_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/language.pb.cc")
set(lang_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/language.pb.h")
set(lang_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/language.grpc.pb.cc")
set(lang_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/language.grpc.pb.h")
add_custom_command(
    OUTPUT "${lang_proto_srcs}" "${lang_proto_hdrs}" "${lang_grpc_srcs}" "${lang_grpc_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
    -I "${catena_proto_path}"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${lang_proto}"
    DEPENDS "${lang_proto}")

# lang_grpc_proto
add_library(lang_grpc_proto
${lang_grpc_srcs}
${lang_grpc_hdrs}
${lang_proto_srcs}
${lang_proto_hdrs})
target_link_libraries(lang_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})
