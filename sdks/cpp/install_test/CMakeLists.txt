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
# cmake build file for Catena C++ examples.
#
#

cmake_minimum_required(VERSION 3.20)

project(install_test C CXX)

get_filename_component(INSTALL_TEST "${CMAKE_SOURCE_DIR}/test_install_dir" ABSOLUTE)
list(APPEND CMAKE_PREFIX_PATH "${INSTALL_TEST}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

find_package(Catena_cpp CONFIG REQUIRED)
message(STATUS "Catena schema: ${CATENA_SCHEMA}")
message(STATUS "Catena codegen: ${CATENA_CODEGEN}") 

message(STATUS "Using protobuf ${Protobuf_VERSION}")
message(STATUS "Using gRPC ${gRPC_VERSION}")

add_subdirectory("status_update")




