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

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${CMAKE_OSX_SYSROOT}/System/Library/Frameworks")
endif(APPLE)

set(target catena_common)

set(sources "src/utils.cpp" "src/vdk/signals.cpp" "src/Path.cpp")
add_library(${target} STATIC ${sources})

target_include_directories(
    ${target}
    PUBLIC
        $<BUILD_INTERFACE:${CATENA_CPP_ROOT_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(${target} PUBLIC cxx_std_20)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Catena_cpp/common
)

install(
    TARGETS ${target}
    EXPORT Catena_cppTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

add_subdirectory(examples)
