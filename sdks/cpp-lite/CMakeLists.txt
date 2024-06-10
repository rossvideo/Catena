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

project(CATENA_CPP_LITE_SDK VERSION 0.0.1 LANGUAGES C CXX)

# export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Assumption: gRPC and all its dependencies are already installed
# on this system, so they can be located by find_package().

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.

if (WIN32)
# Windows toolchain and prefix_path are set through cl args
else()
set(GRPC_LIB_CMAKE "$ENV{HOME}/.local/lib/cmake")
set(CMAKE_PREFIX_PATH "${GRPC_LIB_CMAKE}/protobuf;${GRPC_LIB_CMAKE}/grpc;${GRPC_LIB_CMAKE}/grpc/modules;${GRPC_LIB_CMAKE}/absl;${GRPC_LIB_CMAKE}/utf8_range")
endif(WIN32)

if (APPLE)
  set(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
  set(CMAKE_OSX_FRAMEWORK_PATH "${CMAKE_OSX_SYSROOT}/System/Library/Frameworks")
endif(APPLE)

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

# Find the C preprocessor
find_program(CPP cpp)

# Identify Catena proto folder as interface variable
#
set (relative_interface "../../interface/")
cmake_path(ABSOLUTE_PATH relative_interface OUTPUT_VARIABLE interface)
cmake_path(SET catena_interface NORMALIZE ${interface})

# this is the list of proto files to convert to implementations in
# various languages
#
set(proto_stems 
  "language"
  "param"
  "externalobject"
  "constraint"
  "device"
  "menu"
  "service"
)

set(proto_sources "")
foreach(_proto ${proto_stems})
  # preprocess proto files
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.proto"
    COMMAND ${CPP} -P -C -D OPTIMIZE_FOR=LITE_RUNTIME -o "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.proto" "${catena_interface}${_proto}.proto"
    DEPENDS "${catena_interface}${_proto}.proto"
    COMMENT "Preprocessing ${catena_interface}${_proto}.proto"
    VERBATIM
  )
  # keep a list of preprocessed protobuf source files
  set(proto_sources ${proto_sources} "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.proto")
endforeach()

# identify the target file names that implement the messages and services
# set a custom rule for each proto source to generate the implementations
#
set(sources "")
foreach(_proto ${proto_stems})
  # put names of generated source & header files for _proto stem
  # into local variable _sources
  #
  set(proto_cc "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.pb.cc")
  set(proto_h "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.pb.h")
  set(proto_grpc_cc "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.grpc.pb.cc")
  set(proto_grpc_h "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.grpc.pb.h")
  set(_sources "${proto_cc}" "${proto_h}" "${proto_grpc_cc}" "${proto_grpc_h}")

  # accumulate list of all sources
  set(sources ${sources} ${_sources})
  
  # message(STATUS "sources for ${_proto}: ${_sources}")
  add_custom_command(
    OUTPUT ${_sources}
    PRE_BUILD
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
    -I "${CMAKE_CURRENT_BINARY_DIR}"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.proto"
    DEPENDS ${proto_sources})
endforeach()

# the catena interface library
add_library(catena_interface ${sources})
target_link_libraries(catena_interface
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# add clean target for the protobuf generated files
add_custom_target(clean-proto
  COMMAND ${CMAKE_COMMAND} -E rm ${sources}
  COMMENT "Removing generated protobuf files.")

# include the common part of the Catena C++ SDK
add_subdirectory("common")
