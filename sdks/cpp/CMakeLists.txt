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
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# Catena C++ SDK Main CMake Configuration
# 
# This is the main CMake file for the Catena C++ SDK. It orchestrates the build process
# by including modular configuration files and setting up the build environment.
#
# Usage:
#   cmake -DCONNECTIONS="gRPC;REST" -DCMAKE_BUILD_TYPE=Release <source_dir>
#
# Key Options:
#   - CONNECTIONS: List of connection types to enable (gRPC, REST)  
#   - UNIT_TESTING: Enable/disable unit tests (default: ON)
#   - ONLY_DOCS: Build only documentation (default: OFF)
#

cmake_minimum_required(VERSION 3.20)

# Project declaration
project(Catena_cpp VERSION 0.0.1 LANGUAGES C CXX)

# Add cmake modules to path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#
# Phase 1: Project Configuration
#
include(ProjectConfig)
configure_catena_project()

# Early exit for documentation-only builds
if(ONLY_DOCS)
    include(${CATENA_CPP_ROOT_DIR}/CatenaFunctions.cmake)
    build_docs()
    return()
endif()

#
# Phase 2: Platform and Compiler Setup  
#
include(PlatformConfig)
setup_platform_config()
setup_compiler_config()

#
# Phase 3: External Dependencies
#
include(ExternalDependencies)

#
# Phase 4: Connection Type Configuration
#
# Include core functions first (needed by connection setup)
include(CatenaFunctions)
# Now include connection-specific setup
include(ConnectionTypes)

#
# Phase 5: Core Build Functions
#
# (CatenaFunctions.cmake already included in Phase 4)

#
# Phase 6: Build Core Libraries and Components
#
add_subdirectory("common")
add_subdirectory("connections")

#
# Phase 7: Package and Installation Setup
#
include(${CATENA_CPP_ROOT_DIR}/CatenaCodegen.cmake)
install_catena_codegen()

# Define standard installation directories
include(GNUInstallDirs)

# Export targets
install(
    EXPORT Catena_cppTargets
    FILE Catena_cppTargets.cmake
    NAMESPACE Catena_cpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Catena_cpp
)

# Package configuration
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

#
# Phase 8: Documentation and Testing
#
build_docs()

if(UNIT_TESTING)
    enable_testing()
    set(CATENA_UNITTESTS_DIR ${CMAKE_SOURCE_DIR}/../../unittests)
    message(STATUS "Adding unittests from: ${CATENA_UNITTESTS_DIR}/cpp/")
    message(STATUS "Binary dir for unittests: ${CMAKE_BINARY_DIR}/unittests/")
    add_compile_definitions(CATENA_UNITTESTS_DIR="${CATENA_UNITTESTS_DIR}")
    add_subdirectory("${CATENA_UNITTESTS_DIR}/cpp/" "${CMAKE_BINARY_DIR}/unittests/")
endif()

#
# Build Summary
#
message(STATUS "========================================")
message(STATUS "Catena C++ SDK Build Configuration:")
message(STATUS "  Version: ${CATENA_CPP_VERSION}")
message(STATUS "  Connections: ${CONNECTIONS}")
message(STATUS "  Unit Testing: ${UNIT_TESTING}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "========================================")