# Copyright © 2024 Ross Video Ltd
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

project(gRPC C CXX)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${CMAKE_OSX_SYSROOT}/System/Library/Frameworks")
endif(APPLE)

set(target catena_connections_grpc)

set(sources "src/PeerInfo.cpp" "src/ServiceImpl.cpp")
add_library(${target} STATIC ${sources})

find_package(jwt-cpp CONFIG REQUIRED)

target_include_directories(
    ${target}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Catena_cpp>
)

target_link_libraries(${target}
    PUBLIC
        catena_lite
        proto_interface_${CATENA_MODEL}_grpc
        jwt-cpp::jwt-cpp
)

target_compile_features(${target} PUBLIC cxx_std_20)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Catena_cpp
)

install(TARGETS ${target} 
    EXPORT Catena_cppTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Catena_cpp
)

add_subdirectory(examples)
