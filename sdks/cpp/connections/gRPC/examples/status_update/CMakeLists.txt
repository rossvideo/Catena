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
# cmake build file for the read_model_from_file example.
#
#

cmake_minimum_required(VERSION 3.20)



get_filename_component(TARGET ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${TARGET} C CXX)

add_executable(${TARGET})

get_filename_component(BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../ ABSOLUTE)

message("grpc dir: "${CATENA_CONNECTIONS_GRPC}/include)

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CATENA_COMMON_INCLUDE}>
        $<BUILD_INTERFACE:${CATENA_LITE_INCLUDE}>
        $<BUILD_INTERFACE:${CATENA_CONNECTIONS_GRPC}/include>
        $<BUILD_INTERFACE:${PARENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_PREFIX_PATH}>
)

target_sources(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.cpp>
)

target_link_libraries(${TARGET}
    catena_lite
    catena_lite_grpc
    catena_common
)

target_compile_features(${TARGET}
    PUBLIC
        cxx_std_20
)

