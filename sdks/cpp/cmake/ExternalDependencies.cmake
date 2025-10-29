# Copyright 2024 Ross Video Ltd
# External dependencies management for Catena C++ SDK

cmake_minimum_required(VERSION 3.20)

# Setup NPM and Node.js tools
find_program(NPM_EXECUTABLE npm REQUIRED)

# Tools npm install - only when package.json changes
add_custom_command(
    OUTPUT ${CATENA_ROOT_DIR}/tools/node_modules.stamp
    COMMAND ${NPM_EXECUTABLE} install --yes
    COMMAND ${CMAKE_COMMAND} -E touch ${CATENA_ROOT_DIR}/tools/node_modules.stamp
    WORKING_DIRECTORY ${CATENA_ROOT_DIR}/tools
    DEPENDS ${CATENA_ROOT_DIR}/tools/codegen/package.json
    COMMENT "Installing Catena codegen node modules"
)
add_custom_target(npm_tools DEPENDS ${CATENA_ROOT_DIR}/tools/node_modules.stamp)

# SMPTE tools npm install - only when package.json changes
add_custom_command(
    OUTPUT ${CATENA_ROOT_DIR}/tools/smpte_node_modules.stamp
    COMMAND ${NPM_EXECUTABLE} install --yes
    COMMAND ${CMAKE_COMMAND} -E touch ${CATENA_ROOT_DIR}/tools/smpte_node_modules.stamp
    WORKING_DIRECTORY ${CATENA_ROOT_DIR}/smpte/tools
    DEPENDS ${CATENA_ROOT_DIR}/smpte/tools/package.json
    COMMENT "Installing SMPTE tools node modules"
)
add_custom_target(npm_smpte_tools DEPENDS ${CATENA_ROOT_DIR}/tools/smpte_node_modules.stamp)

# Setup OpenAPI generation with proper dependencies
add_custom_command(
    OUTPUT ${CATENA_ROOT_DIR}/tools/openapi.stamp
    COMMAND bash ${CATENA_ROOT_DIR}/smpte/build-openapi.sh
    COMMAND ${CMAKE_COMMAND} -E touch ${CATENA_ROOT_DIR}/tools/openapi.stamp
    WORKING_DIRECTORY ${CATENA_ROOT_DIR}/smpte
    DEPENDS
        ${CATENA_ROOT_DIR}/smpte/build-openapi.sh
        ${CATENA_ROOT_DIR}/tools/smpte_node_modules.stamp
        ${CATENA_ROOT_DIR}/smpte/interface/openapi/openapi.yaml
    COMMENT "Running build-openapi.sh to generate OpenAPI files"
)

add_custom_target(build_openapi DEPENDS ${CATENA_ROOT_DIR}/tools/openapi.stamp)

# To ensure build_openapi runs first in Ninja, add this to each target:
# add_dependencies(<your_target> build_openapi)
# Example for hello_world:
# add_dependencies(hello_world build_openapi)

# Setup codegen tools
get_filename_component(CODEGEN_DIR "${CATENA_ROOT_DIR}/tools/codegen" ABSOLUTE)
get_filename_component(SCHEMA_LOCATION "${CATENA_ROOT_DIR}/smpte/interface/schemata" ABSOLUTE)

# Install node modules if not present
if(NOT EXISTS "${CODEGEN_DIR}/node_modules")
    find_program(NPM npm REQUIRED)
    message(STATUS "Installing Catena codegen node modules")
    execute_process(
        COMMAND ${NPM} install
        WORKING_DIRECTORY "${CODEGEN_DIR}"
    )
endif()

# Set global variables for submodule compatibility
set(CATENA_CODEGEN "${CODEGEN_DIR}" CACHE STRING "Path to Catena codegen tools" FORCE)
set(CATENA_SCHEMA "${SCHEMA_LOCATION}" CACHE STRING "Path to Catena schema file" FORCE)

# Core dependencies
find_package(absl REQUIRED LOGGING)


# Additional dependencies
find_package(PkgConfig REQUIRED)

# Avahi for mDNS/DNS-SD
pkg_check_modules(AVAHI_CLIENT REQUIRED avahi-client)

# CURL for HTTP client functionality
find_package(CURL REQUIRED)

# Try to find Protobuf using CONFIG first, then MODULE
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
    find_package(Protobuf MODULE REQUIRED)
endif()
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(protobuf_MODULE_COMPATIBLE TRUE)