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
# cmake build file for the read_model_from_file example.
#
#

cmake_minimum_required(VERSION 3.20)


get_filename_component(TARGET ParamAccessorTest NAME)
project(${TARGET} C CXX)

# Include GoogleTest
find_package(GTest REQUIRED)

add_executable(${TARGET})

get_filename_component(BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../ ABSOLUTE)



target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CATENA_COMMON_INCLUDE}>
        $<BUILD_INTERFACE:${PARENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_PREFIX_PATH}>
)

target_sources(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.cpp>
)

target_link_libraries(${TARGET}
    catena_interface;
    GTest::GTest;
)

add_test(ParamAccessor_gtest ${TARGET})

target_compile_features(${TARGET}
    PUBLIC
        cxx_std_20
)