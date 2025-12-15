# Copyright 2025 Ross Video Ltd
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

get_filename_component(TARGET ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${TARGET} C CXX)

add_executable(${TARGET})

# ---- FFmpeg detection (pkg-config preferred, fallback to manual libs) ----
find_package(PkgConfig)
set(FFMPEG_FOUND FALSE)
if(PkgConfig_FOUND)
    pkg_check_modules(FFMPEG IMPORTED_TARGET
        libavformat
        libavcodec
        libavutil
        libswscale
    )
    if(FFMPEG_FOUND)
        message(STATUS "FFmpeg found via pkg-config")
    endif()
endif()

if(NOT FFMPEG_FOUND)
    message(WARNING "FFmpeg pkg-config packages not found. Falling back to manual library linking.\n"
                    "Install dev packages for best results: libavformat-dev libavcodec-dev libavutil-dev")
    # Try to locate libraries manually; rely on linker search path.
    # Users may need to install: libavformat-dev libavcodec-dev libavutil-dev
    find_library(AVFORMAT_LIB avformat)
    find_library(AVCODEC_LIB  avcodec)
    find_library(AVUTIL_LIB   avutil)
    find_library(SWSCALE_LIB  swscale)
    if(AVFORMAT_LIB AND AVCODEC_LIB AND AVUTIL_LIB AND SWSCALE_LIB)
        set(FFMPEG_LIBS ${AVFORMAT_LIB} ${AVCODEC_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB})
        message(STATUS "FFmpeg libs found: ${FFMPEG_LIBS}")
        set(FFMPEG_INCLUDE_DIRS "")
        set(FFMPEG_FOUND TRUE)
    else()
        message(FATAL_ERROR "FFmpeg libraries not found. Please install libavformat-dev libavcodec-dev libavutil-dev libswscale-dev or provide pkg-config files.")
    endif()
endif()

include (${CATENA_CPP_ROOT_DIR}/CatenaCodegen.cmake)

generate_catena_device(
    DEVICE_MODEL_YAML ${CMAKE_CURRENT_SOURCE_DIR}/device.${TARGET}.yaml
    TARGET ${TARGET}
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CATENA_CPP_ROOT_DIR}/common/include
        ${CATENA_CPP_ROOT_DIR}/../../mxl/include
        ${CATENA_CPP_ROOT_DIR}/../../mxl/ndi/include
        # FFmpeg headers (set by pkg-config when available)
        ${FFMPEG_INCLUDE_DIRS}
)

target_sources(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.cpp>
)

target_link_libraries(${TARGET}
    catena_connections_grpc
    absl::flags_parse

    # FFmpeg libs (pkg-config targets if present, else manual)
    $<$<BOOL:$<TARGET_EXISTS:PkgConfig::libavformat>>:PkgConfig::libavformat>
    $<$<BOOL:$<TARGET_EXISTS:PkgConfig::libavcodec>>:PkgConfig::libavcodec>
    $<$<BOOL:$<TARGET_EXISTS:PkgConfig::libavutil>>:PkgConfig::libavutil>
    $<$<BOOL:$<TARGET_EXISTS:PkgConfig::libswscale>>:PkgConfig::libswscale>
    ${FFMPEG_LIBS}
    avformat
    avcodec
    avutil
    swscale
)

# Link all MXL shared libraries from the mxl/app folder
file(GLOB MXL_SO_FILES
    "${CATENA_CPP_ROOT_DIR}/../../mxl/app/*.so"
)

# If any .so files are present, link them directly and ensure the runtime can find them
if(MXL_SO_FILES)
    target_link_libraries(${TARGET} ${MXL_SO_FILES})
endif()

file(GLOB NDI_SO_FILES
    "${CATENA_CPP_ROOT_DIR}/../../mxl/ndi/lib/*.so"
)
if (NDI_SO_FILES)
    target_link_libraries(${TARGET} ${NDI_SO_FILES})
endif()

target_compile_features(${TARGET}
    PUBLIC
        cxx_std_20
)
