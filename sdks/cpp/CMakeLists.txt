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

cmake_minimum_required(VERSION 3.20)

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

# Identify Catena proto folder as interface variable
#
set (relative_interface "../../interface/")
cmake_path(ABSOLUTE_PATH relative_interface OUTPUT_VARIABLE interface)
cmake_path(SET catena_interface NORMALIZE ${interface})
message(STATUS "*** catena_interface = ${catena_interface}")

set(proto_stems 
  "language"
  "param"
  "eo"
  "command"
  "constraint"
  "device"
  "menus"
  "service"
)

message(STATUS "*** proto_stems = ${proto_stems}")
set(sources "")
foreach(_proto ${proto_stems})
  # generated source & header files for _proto stem
  set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.pb.cc")
  set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.pb.h")
  set(proto_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.grpc.pb.cc")
  set(proto_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.grpc.pb.h")
  set(sources ${sources} "${proto_srcs}" "${proto_hdrs}" "${proto_grpc_srcs}" "${proto_grpc_hdrs}")

  message(STATUS "*** ${sources}")
  add_custom_command(
    OUTPUT "${proto_srcs}" "${proto_hdrs}" "${proto_grpc_srcs}" "${proto_grpc_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
    -I "${catena_interface}"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${catena_interface}${_proto}.proto"
    DEPENDS "${catena_interface}${_proto}.proto")
endforeach()


# lang_grpc_proto
add_library(catena_interface ${sources})
target_link_libraries(catena_interface
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})
