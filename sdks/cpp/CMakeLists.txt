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
  find_program(_PROTOBUF_PROTOC protoc)
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
message(STATUS "Models: ${CATENA_MODELS}")
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

# If no models are specified, default to "lite"
if (NOT CATENA_MODELS)
  set(CATENA_MODELS "lite")
endif()
set(ALLOWED_MODELS "full" "lite")


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

foreach(_model ${CATENA_MODELS})
  # Check if the model is valid
  if (NOT _model IN_LIST ALLOWED_MODELS)
    message(FATAL_ERROR "Error: Unknown Catena Model specified: ${_model}")
  endif()

  if (gRPC_enabled)
    set(proto_interface "proto_interface_${_model}_grpc")
  else()
    set(proto_interface "proto_interface_${_model}")
  endif()
  message(STATUS "Building library: ${proto_interface}")

  add_library(${proto_interface} STATIC)

  # preprocess the proto files and add them to proto_interface
  preprocess_protobuf_files("${catena_interface_dir}" "${proto_stems}" "${proto_interface}" "${_model}")

  # use the protobuf generate function provided by the protobuf package
  protobuf_generate(
    TARGET ${proto_interface}
    APPEND_PATH FALSE
    PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/${_model}
    IMPORT_DIRS ${CMAKE_BINARY_DIR}/${_model}
    OUT_VAR cpp_sources
  )

  if(gRPC_enabled)
    # generate the gRPC sources
    protobuf_generate(
      PROTOS ${CMAKE_BINARY_DIR}/${_model}/service.proto
      LANGUAGE grpc
      GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
      APPEND_PATH FALSE
      PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/${_model}
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

  # install the header files
  install(FILES ${header_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Catena_cpp/${_model})
  
  
  # Set the library's output directory
  set_target_properties(${proto_interface} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_model}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_model}"
  )

  if (gRPC_enabled)
    target_link_libraries(${proto_interface} ${_GRPC_GRPCPP})

    if (${_model} STREQUAL "full")
      target_link_libraries(${proto_interface} ${_REFLECTION})
    endif()
  endif()

  if (${_model} STREQUAL "full")
    target_link_libraries(${proto_interface}  ${_PROTOBUF_LIBPROTOBUF})
  else()
    target_link_libraries(${proto_interface} ${_PROTOBUF_LIBPROTOBUF_LITE})
  endif()
  
  target_include_directories(${proto_interface} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/${model}>
    $<BUILD_INTERFACE:${PROTOBUF_INCLUDE_DIRS}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Catena_cpp/${_model}>
  )

  target_compile_features(${proto_interface} PUBLIC cxx_std_20)

  # install the library
  install(TARGETS ${proto_interface}
    EXPORT Catena_cppTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
  
endforeach()

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

#install the Catena device validation schema file
install(
  FILES ${CATENA_ROOT_DIR}/schema/catena.schema.json
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/Catena_cpp
)
set(CATENA_SCHEMA_JSON ${CMAKE_INSTALL_DATAROOTDIR}/Catena_cpp/catena.schema.json)

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
  PATH_VARS CATENA_SCHEMA_JSON CATENA_CODEGEN_INSTALL_DIR
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
