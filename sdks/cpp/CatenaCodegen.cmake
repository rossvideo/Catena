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
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
# Function definition for Catena Code Generator
#
#

cmake_minimum_required(VERSION 3.20)

# Function to generate Catena device source code from a device model file (JSON or YAML)
# Arguments: 
# . DEVICE_MODEL_JSON (Required) location of the device model json file
# . DEVICE_MODEL_YAML (Required) location of the device model yaml file
#   Note: At least ONE of DEVICE_MODEL_JSON or DEVICE_MODEL_YAML must be provided
# . TARGET (Optional) the target to add the generated files to
# . OUT_DIR (Optional) the directory that the generated files will be placed in, defaults to the current binary directory
# . HDR_OUT_VAR (Optional) the variable to store the generated header file
# . SRC_OUT_VAR (Optional) the variable to store the generated source file
#
# Returns:
#  ${HDR_OUT_VAR} the location of the generated header file
#  ${SRC_OUT_VAR} the location of the generated source file
#
function(generate_catena_device)
    
    set(_options)
    set(_singleargs DEVICE_MODEL_JSON DEVICE_MODEL_YAML TARGET OUT_DIR HDR_OUT_VAR SRC_OUT_VAR)
    set(_multiargs IMPORTED_PARAMS)

    cmake_parse_arguments(generate_catena_device "${_options}" "${_singleargs}" "${_multiargs}" "${ARGN}")

    set(_DEVICE_MODEL_JSON ${generate_catena_device_DEVICE_MODEL_JSON})
    set(_DEVICE_MODEL_YAML ${generate_catena_device_DEVICE_MODEL_YAML})
    set(_TARGET ${generate_catena_device_TARGET})
    set(_OUT_DIR ${generate_catena_device_OUT_DIR})
    set(_HDR_OUT_VAR ${generate_catena_device_HDR_OUT_VAR})
    set(_SRC_OUT_VAR ${generate_catena_device_SRC_OUT_VAR})
    set(_IMPORTED_PARAMS ${generate_catena_device_IMPORTED_PARAMS})

    if(NOT DEFINED _TARGET AND (NOT DEFINED _HDR_OUT_VAR OR NOT DEFINED _SRC_OUT_VAR))
        message(SEND_ERROR "Error: generate_catena_device called without any targets or output variables")
        return()
    endif()

    if(NOT DEFINED _DEVICE_MODEL_JSON AND NOT DEFINED _DEVICE_MODEL_YAML)
        message(SEND_ERROR "Error: generate_catena_device called without a device model file")
        return()
    endif()

    if(DEFINED _DEVICE_MODEL_JSON AND DEFINED _DEVICE_MODEL_YAML)
        message(SEND_ERROR "Error: generate_catena_device called with both JSON and YAML device model files")
        return()
    endif()

    if(NOT DEFINED _OUT_DIR)
        # default output directory to current binary directory
        set(_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    # Use either JSON or YAML file
    if(DEFINED _DEVICE_MODEL_JSON)
        set(_DEVICE_MODEL ${_DEVICE_MODEL_JSON})
    else()
        set(_DEVICE_MODEL ${_DEVICE_MODEL_YAML})
    endif()

    get_filename_component(device_name ${_DEVICE_MODEL} NAME)

    set(HEADER ${device_name}.h)
    set(BODY ${device_name}.cpp)

    # Get all files in the CODEGEN_DIR
    file(GLOB CODEGEN_FILES "${CATENA_CODEGEN}/*.js" "${CATENA_CODEGEN}/cpp/*.js")
    file(GLOB SCHEMA_FILES "${CATENA_SCHEMA}/*.json")

    # convert json/yaml to cpp
    find_program(NODE node REQUIRED)
    add_custom_command(
        OUTPUT ${_OUT_DIR}/${HEADER}
            ${_OUT_DIR}/${BODY}
        COMMAND ${NODE} ${CATENA_CODEGEN} --schema ${CATENA_SCHEMA} --device-model "${_DEVICE_MODEL}" --output ${_OUT_DIR}
        DEPENDS ${_DEVICE_MODEL} ${SCHEMA_FILES} ${CODEGEN_FILES} ${_IMPORTED_PARAMS}
        COMMENT "Generating ${HEADER} and ${BODY} from ${_DEVICE_MODEL}"
    )

    set_source_files_properties(${_OUT_DIR}/${HEADER} ${_OUT_DIR}/${BODY} PROPERTIES GENERATED TRUE)

    if(DEFINED _TARGET)
        target_sources(${_TARGET} PRIVATE ${_OUT_DIR}/${BODY})
        target_include_directories(${_TARGET} PRIVATE ${_OUT_DIR})
    endif()

    if(DEFINED _HDR_OUT_VAR)
        set(${generate_catena_device_HDR_OUT_VAR} ${_OUT_DIR}/${HEADER} PARENT_SCOPE)
    endif()
    if(DEFINED _SRC_OUT_VAR)
        set(${generate_catena_device_SRC_OUT_VAR} ${_OUT_DIR}/${BODY} PARENT_SCOPE)
    endif()

endfunction()