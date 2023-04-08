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
#

cmake_minimum_required(VERSION 3.20)

project(COMMON C CXX)

set(LIB_TARGET catena_common)
set(LIB_SRC_DIR "src")
set(LIB_INCLUDE_DIR "include")

add_library(${LIB_TARGET})


get_filename_component(PARENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../ ABSOLUTE)
message(STATUS "*** binary dir: ${PARENT_BINARY_DIR}")
target_include_directories(${LIB_TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${LIB_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${PARENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_PREFIX_PATH}>
)

target_sources(${LIB_TARGET}
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${LIB_SRC_DIR}/utils.cpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${LIB_SRC_DIR}/DeviceModel.cpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${LIB_SRC_DIR}/Path.cpp>
)

target_link_libraries(${LIB_TARGET}
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
)

target_compile_features(${LIB_TARGET}
    PUBLIC
        cxx_std_17
)

add_subdirectory("examples")