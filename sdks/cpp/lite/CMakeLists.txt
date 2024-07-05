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

project(LITE C CXX)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${CMAKE_OSX_SYSROOT}/System/Library/Frameworks")
endif(APPLE)


set(target catena_lite)

add_library(${target} STATIC 
    src/Device.cpp
    src/Param.cpp
    src/StructInfo.cpp
)

add_dependencies(${target} catena_common)
add_dependencies(${target} ${proto_interface})

target_include_directories(
    ${target}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CATENA_CPP_ROOT_DIR}>
        $<BUILD_INTERFACE:${CMAKE_PREFIX_PATH}>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
        $<BUILD_INTERFACE:${GRPC_INSTALL_INCLUDE_DIR}>
)

target_link_libraries(${target}
    catena_common
)

target_compile_features(${target} PUBLIC cxx_std_20)

add_subdirectory(examples)

