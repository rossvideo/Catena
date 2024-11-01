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
# cmake build file for the read_model_from_file example.
#
#

cmake_minimum_required(VERSION 3.20)

project(REST C CXX)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${CMAKE_OSX_SYSROOT}/System/Library/Frameworks")
endif(APPLE)

set(target catena_connections_REST)
set(REST_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXTERNAL_ROOT_DIR ${REST_ROOT_DIR}/external)
set(CROW_ROOT_DIR  ${EXTERNAL_ROOT_DIR}/crow)
set(BOOST_ROOT_DIR ${EXTERNAL_ROOT_DIR}/boost)


set(sources "src/api.cpp")
add_library(${target} STATIC ${sources})

find_package(jwt-cpp CONFIG REQUIRED)

set(ASIO_INCLUDE_DIR "${BOOST_ROOT_DIR}/asio/include")


# Specify include directories for each INTERFACE library
include_directories(
    ${BOOST_ROOT_DIR}/asio/include
    ${BOOST_ROOT_DIR}/config/include
    ${BOOST_ROOT_DIR}/core/include
    ${BOOST_ROOT_DIR}/predef/include
    ${BOOST_ROOT_DIR}/assert/include
    ${BOOST_ROOT_DIR}/throw_exception/include
    ${BOOST_ROOT_DIR}/variant2/include
    ${BOOST_ROOT_DIR}/winapi/include
    ${BOOST_ROOT_DIR}/move/include
    ${CROW_ROOT_DIR}/include
    ${REST_ROOT_DIR}/include
)

# Define non-namespaced INTERFACE targets for Boost dependencies
add_library(boost_assert INTERFACE)
target_include_directories(boost_assert INTERFACE ${BOOST_ROOT_DIR}/assert/include)

add_library(boost_config INTERFACE)
target_include_directories(boost_config INTERFACE ${BOOST_ROOT_DIR}/config/include)

add_library(boost_throw_exception INTERFACE)
target_include_directories(boost_throw_exception INTERFACE ${BOOST_ROOT_DIR}/throw_exception/include)

add_library(boost_variant2 INTERFACE)
target_include_directories(boost_variant2 INTERFACE ${BOOST_ROOT_DIR}/variant2/include)

add_library(boost_winapi INTERFACE)
target_include_directories(boost_winapi INTERFACE ${BOOST_ROOT_DIR}/winapi/include)

# Create ALIAS targets with Boost:: prefixes
add_library(Boost::assert ALIAS boost_assert)
add_library(Boost::config ALIAS boost_config)
add_library(Boost::throw_exception ALIAS boost_throw_exception)
add_library(Boost::variant2 ALIAS boost_variant2)
add_library(Boost::winapi ALIAS boost_winapi)


# Add Crow as a submodule
set(CROW_USE_BOOST ON)
set(BOOST_ROOT "${BOOST_ROOT_DIR}")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")
add_subdirectory(${CROW_ROOT_DIR})

# Add Boost.System as a submodule with add_subdirectory
add_subdirectory(${BOOST_ROOT_DIR}/system)
target_link_libraries(boost_system INTERFACE boost_assert boost_config boost_throw_exception boost_variant2 boost_winapi)

# Link Crow and Boost.System to your target
# todo: add jwt-cpp
target_link_libraries(${target} PRIVATE crow boost_system)

target_compile_features(${target} PUBLIC cxx_std_20)

# todo - add boost_system to the exported targets
# install(
#     DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
#     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Catena_cpp
# )

# install(TARGETS ${target} 
#     EXPORT Catena_cppTargets
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Catena_cpp
# )

add_subdirectory(examples)

