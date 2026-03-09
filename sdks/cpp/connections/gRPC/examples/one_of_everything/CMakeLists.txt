# Copyright 2025 Ross Video Ltd
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

#
# cmake build file for the one_of_everything example.
#

cmake_minimum_required(VERSION 3.20)

get_filename_component(TARGET ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${TARGET} C CXX)

add_executable(${TARGET})

include (${CATENA_CPP_ROOT_DIR}/CatenaCodegen.cmake)

file(GLOB PARAM_EXAMPLES "${CMAKE_CURRENT_SOURCE_DIR}/params/param.*.yaml")
file(GLOB CONSTRAINT_EXAMPLES "${CMAKE_CURRENT_SOURCE_DIR}/constraints/param.*.yaml")

generate_catena_device(
    DEVICE_MODEL_JSON ${CMAKE_CURRENT_SOURCE_DIR}/device.${TARGET}.yaml
    IMPORTED_PARAMS ${PARAM_EXAMPLES} ${CONSTRAINT_EXAMPLES}
    TARGET ${TARGET}
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CATENA_CPP_ROOT_DIR}/common/include
)



target_sources(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.cpp>      
)

target_link_libraries(${TARGET}
    catena_connections_grpc
    absl::flags_parse
)

if(WIN32)
    file(GLOB _msvc_debug_crt_dirs "C:/Program Files (x86)/Microsoft Visual Studio/*/*/VC/Redist/MSVC/*/debug_nonredist/x64/Microsoft.VC*.DebugCRT")
    set(_msvc_debug_crt_dlls)
    foreach(_crt_dir IN LISTS _msvc_debug_crt_dirs)
        file(GLOB _crt_dir_dlls "${_crt_dir}/*.dll")
        list(APPEND _msvc_debug_crt_dlls ${_crt_dir_dlls})
    endforeach()

    set(_ucrt_debug_dll)
    if(EXISTS "C:/Windows/System32/ucrtbased.dll")
        set(_ucrt_debug_dll "C:/Windows/System32/ucrtbased.dll")
    endif()

    set(_vcpkg_triplet "x64-windows")
    if(DEFINED VCPKG_TARGET_TRIPLET)
        set(_vcpkg_triplet "${VCPKG_TARGET_TRIPLET}")
    endif()
    if(NOT _vcpkg_triplet STREQUAL "x64-windows-static")
        file(GLOB _vcpkg_debug_bin_dlls "C:/vcpkg/installed/${_vcpkg_triplet}/debug/bin/*.dll")
        file(GLOB _vcpkg_bin_dlls "C:/vcpkg/installed/${_vcpkg_triplet}/bin/*.dll")
    endif()

    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${TARGET}> $<TARGET_RUNTIME_DLLS:${TARGET}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_msvc_debug_crt_dlls} ${_ucrt_debug_dll} $<TARGET_FILE_DIR:${TARGET}>
        COMMAND_EXPAND_LISTS
    )
    if(NOT _vcpkg_triplet STREQUAL "x64-windows-static")
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_vcpkg_debug_bin_dlls} ${_vcpkg_bin_dlls} $<TARGET_FILE_DIR:${TARGET}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()

target_compile_features(${TARGET}
    PUBLIC
        cxx_std_20
)
