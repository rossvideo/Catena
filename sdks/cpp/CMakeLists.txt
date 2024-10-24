# Copyright 2024 Ross Video Ltd
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
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
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.5" CACHE STRING "Minimum OS X deployment version")

project(Catena_cpp VERSION 0.0.1 LANGUAGES C CXX)

# pull in Catena build functions
include(${CMAKE_CURRENT_SOURCE_DIR}/CatenaFunctions.cmake)

get_filename_component(CODEGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/codegen" ABSOLUTE) 
get_filename_component(SCHEMA_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../../schema/catena.schema.json" ABSOLUTE)

# install node modules if it hasn't been done yet
if(NOT EXISTS "${CODEGEN_DIR}/node_modules")
  find_program(NPM npm REQUIRED)
  message(STATUS "Installing Catena codegen node modules")
  message("codegen_dir: ${CODEGEN_DIR}")
  execute_process(
      COMMAND ${NPM} install
      WORKING_DIRECTORY "${CODEGEN_DIR}"
  )
endif()

# Set these variable globally so that they are available when catena is attached as a submodule
set(CATENA_CODEGEN "${CODEGEN_DIR}" CACHE STRING "Path to Catena codegen tools" FORCE)
set(CATENA_SCHEMA "${SCHEMA_LOCATION}" CACHE STRING "Path to Catena schema file" FORCE)

# export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define standard installation directories for the target system
include(GNUInstallDirs)

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

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_PROTOBUF_LIBPROTOBUF_LITE protobuf::libprotobuf-lite)
set(_REFLECTION gRPC::grpc++_reflection)
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

# Check that a valid model is specified
if ((NOT "${CATENA_MODEL}" STREQUAL "full") AND (NOT "${CATENA_MODEL}" STREQUAL "lite"))
  message(FATAL_ERROR "${CATENA_MODEL} is not a valid model. CATENA_MODEL must be either 'full' or 'lite'.")
endif()
message(STATUS "Model: ${CATENA_MODEL}")

# Check for invalid connection types
set(ALLOWED_CONNECTIONS "gRPC")
foreach(conn ${CONNECTIONS})
  list(FIND ALLOWED_CONNECTIONS "${conn}" _IDX)
  if(${_IDX} EQUAL -1)
    message(FATAL_ERROR "Error: '${conn}' is not a valid connection type. Allowed types are ${ALLOWED_CONNECTIONS}.")
  endif()
endforeach()

# create flags _gRPC, _REST, _WSS to indicate which connections are enabled
message(STATUS "Connections: ${CONNECTIONS}")
list(FIND CONNECTIONS "gRPC" _gRPC)
list(FIND CONNECTIONS "REST" _REST)
list(FIND CONNECTIONS "WSS" _WSS)
if (${_gRPC} GREATER -1)
  message(STATUS "gRPC connection enabled")
  set(gRPC_enabled TRUE)
endif()
if (${_REST} GREATER -1)
  message(STATUS "REST connection enabled")
  set(REST_enabled TRUE)
endif()
if (${_WSS} GREATER -1)
  message(STATUS "WSS connection enabled")
  set(WSS_enabled TRUE)
endif()

# Check that at least one connection type is specified
if (NOT (gRPC_enabled OR REST_enabled OR WSS_enabled))
  message(FATAL_ERROR "Error: At least one connection type must be specified.")
endif()

# Identify Catena proto folder as interface variable
set (relative_interface "../../interface/")
cmake_path(ABSOLUTE_PATH relative_interface OUTPUT_VARIABLE interface)
cmake_path(SET catena_interface_dir NORMALIZE ${interface})
message(STATUS "Catena interface directory: ${catena_interface_dir}")

# this is the list of proto files to convert to implementations in
# various languages
set(proto_stems 
  "language"
  "param"
  "externalobject"
  "constraint"
  "device"
  "menu"
)
if(gRPC_enabled)
  list(APPEND proto_stems "service")
endif()
message(STATUS "proto_stems: ${proto_stems}")


if (gRPC_enabled)
  set(proto_interface "proto_interface_${CATENA_MODEL}_grpc")
else()
  set(proto_interface "proto_interface_${CATENA_MODEL}")
endif()
message(STATUS "Building library: ${proto_interface}")

add_library(${proto_interface} STATIC)

# preprocess the proto files and add them to proto_interface
preprocess_protobuf_files("${catena_interface_dir}" "${proto_stems}" "${proto_interface}" "${CATENA_MODEL}")

# use the protobuf generate function provided by the protobuf package
protobuf_generate(
  TARGET ${proto_interface}
  APPEND_PATH FALSE
  PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/interface
  IMPORT_DIRS ${CMAKE_BINARY_DIR}/${CATENA_MODEL}
  OUT_VAR cpp_sources
)

if(gRPC_enabled)
  # generate the gRPC sources
  protobuf_generate(
    PROTOS ${CMAKE_BINARY_DIR}/${CATENA_MODEL}/service.proto
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    APPEND_PATH FALSE
    PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/interface
    PLUGIN "protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    OUT_VAR output
  )
  target_sources(${proto_interface} PRIVATE ${output})
  list(APPEND cpp_sources ${output})
endif()

# Filter header files from cpp_sources
set(header_files)
foreach(file ${cpp_sources})
  if(file MATCHES "\\.h$")
    list(APPEND header_files ${file})
  endif()
endforeach()

# Set the library's output directory
set_target_properties(${proto_interface} PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CATENA_MODEL}"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CATENA_MODEL}"
)

if (gRPC_enabled)
  target_link_libraries(${proto_interface} PUBLIC ${_GRPC_GRPCPP})

  if (${CATENA_MODEL} STREQUAL "full")
    target_link_libraries(${proto_interface} PUBLIC ${_REFLECTION})
  endif()
endif()

if (${CATENA_MODEL} STREQUAL "full")
  target_link_libraries(${proto_interface} PUBLIC ${_PROTOBUF_LIBPROTOBUF})
else()
  target_link_libraries(${proto_interface} PUBLIC ${_PROTOBUF_LIBPROTOBUF_LITE})
endif()

target_include_directories(${proto_interface} PUBLIC
  $<BUILD_INTERFACE:${PROTOBUF_INCLUDE_DIRS}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Catena_cpp/interface>
)

target_compile_features(${proto_interface} PUBLIC cxx_std_20)

# install the header files
install(FILES ${header_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Catena_cpp/interface)

# install the library
install(TARGETS ${proto_interface}
  EXPORT Catena_cppTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# add clean target for the protobuf generated files
add_custom_target(clean-proto
  COMMAND ${CMAKE_COMMAND} -E rm -f ${full_protos} ${lite_protos} ${full_sources} ${lite_sources}
  COMMENT "Removing generated protobuf files.")

# include the common part of the Catena C++ SDK
add_subdirectory("common")

add_subdirectory("lite")

# include the connections part of the Catena C++ SDK
add_subdirectory("connections")

#
#
# Package and install the Catena C++ SDK

install_catena_codegen()

install(
  EXPORT Catena_cppTargets
  FILE Catena_cppTargets.cmake
  NAMESPACE Catena_cpp::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Catena_cpp
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Catena_cppConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/Catena_cppConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Catena_cpp
  PATH_VARS CATENA_SCHEMA_JSON CATENA_CODEGEN_INSTALL_DIR CATENA_MODEL
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/Catena_cppConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/Catena_cppConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/Catena_cppConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Catena_cpp
)

# Build the documentation with Doxygen
build_docs()
