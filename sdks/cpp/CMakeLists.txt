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

# this is the list of proto files to convert to implementations in
# various languages
#
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

# identify the target file names that implement the messages and services
# set a custom rule for each proto source to generate the implementations
#
set(sources "")
foreach(_proto ${proto_stems})
  # put names of generated source & header files for _proto stem
  # into local variable _sources
  #
  set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.pb.cc")
  set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.pb.h")
  set(_sources "${proto_srcs}" "${proto_hdrs}")

  # the servcice file contains grpc definitions, so a couple of extra
  # file names are required
  #
  if(_proto STREQUAL "service")
    set(proto_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.grpc.pb.cc")
    set(proto_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${_proto}.grpc.pb.h")
    set(_sources ${_sources} "${proto_grpc_srcs}" "${proto_grpc_hdrs}")
  endif()

  # accumulate list of all sources
  set(sources ${sources} ${_sources})
  
  message(STATUS "sources for ${_proto}: ${_sources}")
  add_custom_command(
    OUTPUT ${_sources}
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
    -I "${catena_interface}"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${catena_interface}${_proto}.proto"
    DEPENDS "${catena_interface}${_proto}.proto")
endforeach()

# the catena interface library
add_library(catena_interface ${sources})
target_link_libraries(catena_interface
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# include the common part of the Catena C++ SDK
add_subdirectory("common" EXCLUDE_FROM_ALL)

# doxygen
#
option(BUILD_DOC "Build documentation" ON)

# check if Doxygen is installed
find_package(Doxygen OPTIONAL_COMPONENTS dot)

if (DOXYGEN_FOUND AND BUILD_DOC)
# set input and output files
set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

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
