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

# pull in the protobuf functions
include(${CMAKE_CURRENT_SOURCE_DIR}/ProtobufFunctions.cmake)

# export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(UNIT_TESTING "Build Google test" ON)
if(UNIT_TESTING)
  # enable unit testing
  enable_testing()
endif(UNIT_TESTING)

# Assumption: gRPC and all its dependencies are already installed
# on this system, so they can be located by find_package().


if (WIN32)
  # Windows toolchain and prefix_path are set through command line
  # nothing to do here
else()
  # set options for linux
  set(GRPC_LIB_CMAKE "$ENV{HOME}/.local/lib/cmake")
  set(CMAKE_PREFIX_PATH "${GRPC_LIB_CMAKE}/protobuf;${GRPC_LIB_CMAKE}/grpc;${GRPC_LIB_CMAKE}/grpc/modules;${GRPC_LIB_CMAKE}/absl;${GRPC_LIB_CMAKE}/utf8_range")
endif(WIN32)

if (APPLE)
  # set up for MacOS
  set(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
  set(CMAKE_OSX_FRAMEWORK_PATH "${CMAKE_OSX_SYSROOT}/System/Library/Frameworks")
  set(CPP_OPTS -E -x c -P)
endif (APPLE)

get_filename_component(CATENA_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
set(CATENA_CPP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "CATENA_ROOT_DIR: ${CATENA_ROOT_DIR}")
message(STATUS "CATENA_CPP_ROOT_DIR: ${CATENA_CPP_ROOT_DIR}")

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_PROTOBUF_LIBPROTOBUF_LITE protobuf::libprotobuf-lite)
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

# find the c preprocessor and set the options to use with it
if (APPLE)
  set(CPP clang)
  set(cpp_opts -E -P -x c)
else()
  find_program(CPP cpp)
  set(cpp_opts -P -C)
endif(APPLE)
if (NOT CPP)
  message(FATAL_ERROR "CPP not set")
else()
  message(STATUS "CPP = ${CPP}")
endif()


# Identify Catena proto folder as interface variable
#
set (relative_interface "../../interface/")
cmake_path(ABSOLUTE_PATH relative_interface OUTPUT_VARIABLE interface)
cmake_path(SET catena_interface_dir NORMALIZE ${interface})
message(STATUS "Catena interface directory: ${catena_interface_dir}")

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
)


# create flags _gRPC, _REST, _WSS to indicate which connections are enabled
#
message(STATUS "Connections: ${CONNECTIONS}")
list(FIND CONNECTIONS "gRPC" _gRPC)
list(FIND CONNECTIONS "REST" _REST)
list(FIND CONNECTIONS "WSS" _WSS)
if (${_gRPC} GREATER -1)
  message(STATUS "gRPC connection enabled")
  list(APPEND proto_stems "service")
endif()
if (${_REST} GREATER -1)
  message(STATUS "REST connection enabled")
endif()
if (${_WSS} GREATER -1)
  message(STATUS "WSS connection enabled")
endif()


foreach(_model ${CATENA_MODELS})
  preprocess_protobuf_files(${catena_interface_dir} "${proto_stems}" ${_model})
  generate_cpp_protobuf_sources ("${proto_stems}" ${_model} ${_gRPC})

  set (protos ${${_model}_protos})
  set (sources ${${_model}_sources})
  set (intermediates ${${_model}_intermediates})

  message(STATUS sources: ${sources})
  message(STATUS protos: ${protos})
  message(STATUS intermediates: "${intermediates}")



  if (_gRPC GREATER -1)
    set(libtarget "catena_${_model}_grpc")
  else()
    set(libtarget "catena_${_model}")
  endif()

  set_source_files_properties(${sources} PROPERTIES GENERATED TRUE)
  add_library(${libtarget} STATIC ${sources})
  foreach (target ${intermediates})
    message(STATUS "Adding dependency: ${target}")
    add_dependencies(${libtarget} ${target})
  endforeach()
  
  # Set the library's output directory
  set_target_properties(${libtarget} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_model}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_model}"
  )
  target_include_directories(${libtarget} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/${model} "~/.local/include")
  target_compile_features(${libtarget} PUBLIC cxx_std_20)
  

  if (_gRPC GREATER -1)
    if (${_model} STREQUAL "full")
      target_link_libraries(${libtarget} ${_GRPC_GRPCPP} ${_REFLECTION} ${_PROTOBUF_LIBPROTOBUF})
    else()
      target_link_libraries(${libtarget} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF_LITE})
    endif()
  else()
    if (${_model} STREQUAL "full")
      target_link_libraries(${libtarget}  ${_PROTOBUF_LIBPROTOBUF})
    else()
      target_link_libraries(${libtarget} ${_PROTOBUF_LIBPROTOBUF_LITE})
    endif()
  endif()

endforeach()

# add clean target for the protobuf generated files
add_custom_target(clean-proto
  COMMAND ${CMAKE_COMMAND} -E rm -f ${full_protos} ${lite_protos} ${full_sources} ${lite_sources}
  COMMENT "Removing generated protobuf files.")

# include the common part of the Catena C++ SDK
add_subdirectory("common")

#include the lite part of the Catena C++ SDK
add_subdirectory("lite")

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
