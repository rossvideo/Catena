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

get_filename_component(CATENA_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
set(CATENA_CPP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# pull in Catena build functions
include(${CATENA_CPP_ROOT_DIR}/CatenaFunctions.cmake)

# if only building the documentation, then do that and return
option(ONLY_DOCS "Only build the documentation" OFF)
if (ONLY_DOCS)
  build_docs()
  return()
endif(ONLY_DOCS)

get_filename_component(CODEGEN_DIR "${CATENA_ROOT_DIR}/tools/codegen" ABSOLUTE) 
get_filename_component(SCHEMA_LOCATION "${CATENA_ROOT_DIR}/schema" ABSOLUTE)

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
  # set options for linux, and mac
  # nothing to do here, so why not delete this block?
endif(WIN32)

if (APPLE)
  # set up for MacOS
  set(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
  set(CMAKE_OSX_FRAMEWORK_PATH "${CMAKE_OSX_SYSROOT}/System/Library/Frameworks")
  set(GRPC_LIB_CMAKE "$ENV{HOME}/.local/lib/cmake")
  set(CMAKE_PREFIX_PATH "${GRPC_LIB_CMAKE}/protobuf;${GRPC_LIB_CMAKE}/grpc;${GRPC_LIB_CMAKE}/grpc/modules;${GRPC_LIB_CMAKE}/absl;${GRPC_LIB_CMAKE}/utf8_range")
  include_directories("$ENV{HOME}/.local/include")
  set(CPP_OPTS -E -x c -P)
endif (APPLE)

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

# Check for invalid connection types
set(ALLOWED_CONNECTIONS "gRPC;REST")
foreach(conn ${CONNECTIONS})
  list(FIND ALLOWED_CONNECTIONS "${conn}" _IDX)
  if(${_IDX} EQUAL -1)
    message(FATAL_ERROR "Error: '${conn}' is not a valid connection type. Allowed types are ${ALLOWED_CONNECTIONS}.")
  endif()
endforeach()

# create enabled flags for gRPC, REST and WSS to indicate which connections are enabled
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
  message(ERROR "Error: At least one connection type must be specified.")
  return()
endif()

# find the protobuf package; needed by all connection types
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

# Locate the input proto files
set (relative_interface "../../interface/")
cmake_path(ABSOLUTE_PATH relative_interface OUTPUT_VARIABLE interface)
cmake_path(SET catena_interface_dir NORMALIZE ${interface})

# this is the list of proto files to convert to implementations in
# various languages
set(interface_proto_stems 
  "language"
  "param"
  "externalobject"
  "constraint"
  "device"
  "menu"
)

# conditionally set up the gRPC targets
if (gRPC_enabled)
  include(${CATENA_CPP_ROOT_DIR}/gRPC.cmake)
  set_up_gRPC_targets()
endif(gRPC_enabled)













# set up our target
set(REST_TARGET "rest_proto_interface")

# set up directories for each target
set(REST_INTERFACE_DIR  "${CMAKE_BINARY_DIR}/${REST_TARGET}")

# add_library(${REST_TARGET} STATIC)

# # if (REST_enabled)
# #   preprocess_protobuf_files("${catena_interface_dir}" "${interface_proto_stems}" "${FULL_INTERFACE}" "full")

# #   # use the protobuf generate function provided by the protobuf package to make the c++ source files
# #   protobuf_generate(
# #     TARGET ${FULL_INTERFACE}
# #     APPEND_PATH FALSE
# #     PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/full/interface
# #     IMPORT_DIRS ${CMAKE_BINARY_DIR}/full/
# #     OUT_VAR cpp_sources
# #   )

# # endif()


# # Filter header files from cpp_sources
# set(header_files)
# foreach(file ${cpp_sources})
#   if(file MATCHES "\\.h$")
#     list(APPEND header_files ${file})
#   endif()
# endforeach()



# target_include_directories(${proto_interface} PUBLIC
#   $<BUILD_INTERFACE:${PROTOBUF_INCLUDE_DIRS}>
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
#   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Catena_cpp/interface>
# )

# target_compile_features(${proto_interface} PUBLIC cxx_std_20)

# # install the header files
# install(FILES ${header_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Catena_cpp/interface)

# # install the library
# install(TARGETS ${proto_interface}
#   EXPORT Catena_cppTargets
#   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
# )


# # include the common part of the Catena C++ SDK
# add_subdirectory("common")


# # include the connections part of the Catena C++ SDK
# # add_subdirectory("connections")

# #
# #
# # Package and install the Catena C++ SDK

# install_catena_codegen()

# install(
#   EXPORT Catena_cppTargets
#   FILE Catena_cppTargets.cmake
#   NAMESPACE Catena_cpp::
#   DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Catena_cpp
# )

# include(CMakePackageConfigHelpers)
# configure_package_config_file(
#   ${CMAKE_CURRENT_SOURCE_DIR}/Catena_cppConfig.cmake.in
#   ${CMAKE_CURRENT_BINARY_DIR}/Catena_cppConfig.cmake
#   INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Catena_cpp
#   PATH_VARS CATENA_SCHEMA_JSON CATENA_CODEGEN_INSTALL_DIR CATENA_MODEL
# )

# write_basic_package_version_file(
#   ${CMAKE_CURRENT_BINARY_DIR}/Catena_cppConfigVersion.cmake
#   VERSION ${PROJECT_VERSION}
#   COMPATIBILITY AnyNewerVersion
# )

# install(
#   FILES
#     ${CMAKE_CURRENT_BINARY_DIR}/Catena_cppConfig.cmake
#     ${CMAKE_CURRENT_BINARY_DIR}/Catena_cppConfigVersion.cmake
#   DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Catena_cpp
# )

# Build the documentation with Doxygen
build_docs()

# It's cleaner to use the imported targets, so let's do that
# set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
# set(_PROTOBUF_LIBPROTOBUF_LITE protobuf::libprotobuf-lite)
# set(_REFLECTION gRPC::grpc++_reflection)