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

project(CATENA_FULL_SDK C CXX)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${CMAKE_OSX_SYSROOT}/System/Library/Frameworks")
endif(APPLE)
message(STATUS "*** CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")

set(FULL_TARGET catena_full)
set(FULL_SRC_DIR "src")
get_filename_component(FULL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include ABSOLUTE)

add_library(${FULL_TARGET} STATIC)

add_dependencies(${FULL_TARGET} ${CATENA_INTERFACE_LIB})

get_filename_component(PARENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../ ABSOLUTE)
message(STATUS "*** binary dir: ${PARENT_BINARY_DIR}")
target_include_directories(${FULL_TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${FULL_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${PARENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
        $<BUILD_INTERFACE:${CMAKE_PREFIX_PATH}>
)

target_sources(${FULL_TARGET}
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${FULL_SRC_DIR}/DeviceModel.cpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${FULL_SRC_DIR}/ParamAccessor.cpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${FULL_SRC_DIR}/PeerInfo.cpp>
)

target_link_libraries(${FULL_TARGET}
    ${CATENA_COMMON_LIB}
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
)

target_compile_features(${FULL_TARGET}
    PUBLIC
        cxx_std_20
)

add_subdirectory("examples")
if(UNIT_TESTING)
    add_subdirectory("tests")
endif()