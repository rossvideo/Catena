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

project(CATENA_CPP_SDK VERSION 0.0.1 LANGUAGES C CXX)

# export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(UNIT_TESTING "Build Google test" ON)
if(UNIT_TESTING)
# enable unit testing
enable_testing()
endif(UNIT_TESTING)

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

# check if building full or lite sdk
option(BUILD_LITE "Build lite version of the SDK" OFF)

# Find the C preprocessor
find_program(CPP cpp)

# Identify Catena proto folder as interface variable
#
set (relative_interface "../../interface/")
cmake_path(ABSOLUTE_PATH relative_interface OUTPUT_VARIABLE interface)
cmake_path(SET PROTO_SRC_DIR NORMALIZE ${interface})

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

if(BUILD_LITE)
  set(PROTO_BUILD_MODE "LITE_RUNTIME")
else()
  set(PROTO_BUILD_MODE "CODE_SIZE")
endif(BUILD_LITE)

set(proto_sources "")
foreach(_proto ${proto_stems})
  # preprocess proto files
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.proto"
    COMMAND ${CPP} -P -C -D OPTIMIZE_FOR=${PROTO_BUILD_MODE} -o "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.proto" "${PROTO_SRC_DIR}${_proto}.proto"
    DEPENDS "${PROTO_SRC_DIR}${_proto}.proto"
    COMMENT "Preprocessing ${PROTO_SRC_DIR}${_proto}.proto"
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
set(CATENA_INTERFACE_LIB catena_interface)
add_library(${CATENA_INTERFACE_LIB} ${sources})
target_link_libraries(${CATENA_INTERFACE_LIB}
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)

# parent binary directory where the generated files are stored
get_filename_component(PARENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} ABSOLUTE)
message(STATUS "*** binary dir: ${PARENT_BINARY_DIR}")

# add clean target for the protobuf generated files
add_custom_target(clean-proto
  COMMAND ${CMAKE_COMMAND} -E rm ${sources}
  COMMENT "Removing generated protobuf files.")

# include the common part of the Catena C++ SDK
set(CATENA_COMMON_LIB catena_common)
add_subdirectory("common")

# include the selected version of the Catena C++ SDK
if(BUILD_LITE)
  add_subdirectory("lite")
else()
  add_subdirectory("full")
endif()

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
          VERBATIM)


elseif(BUILD_DOC)
message("Doxygen needs to be installed to generate the documentation")
endif (DOXYGEN_FOUND AND BUILD_DOC)
