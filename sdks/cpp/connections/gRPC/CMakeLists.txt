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

project(gRPC C CXX)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${CMAKE_OSX_SYSROOT}/System/Library/Frameworks")
endif(APPLE)

set(target catena_connections_grpc)

set(sources 
    "src/PeerInfo.cpp"
    "src/ServiceImpl.cpp" 
    "src/GetParam.cpp"
    "src/GetPopulatedSlots.cpp"
    "src/SetValue.cpp"
    "src/GetValue.cpp"
    "src/Connect.cpp"
    "src/DeviceRequest.cpp"
    "src/ExternalObjectRequest.cpp"
    "src/BasicParamInfoRequest.cpp"
    "src/ExecuteCommand.cpp"
    "src/SharedFlags.cpp"
    "src/ServiceCredentials.cpp"
    "src/MultiSetValue.cpp"
    "src/AddLanguage.cpp"
    "src/ListLanguages.cpp"
    "src/LanguagePackRequest.cpp"
)

add_library(${target} STATIC ${sources})

find_package(CURL REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)

target_include_directories(
    ${target}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Catena_cpp>
)

target_link_libraries(${target}
    PUBLIC
        grpc_service
        catena_grpc_common
        CURL::libcurl
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
