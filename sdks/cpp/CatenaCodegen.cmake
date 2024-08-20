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
# Function definition for Catena Code Generator
#
#

cmake_minimum_required(VERSION 3.20)

# Function to generate Catena device source code from a device model json file
# Arguments: 
# . DEVICE_MODEL_JSON (Required) location of the device model json file
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
    set(_singleargs DEVICE_MODEL_JSON TARGET OUT_DIR HDR_OUT_VAR SRC_OUT_VAR)
    set(_multiargs)

    cmake_parse_arguments(generate_catena_device "${_options}" "${_singleargs}" "${_multiargs}" "${ARGN}")

    set(_DEVICE_MODEL_JSON ${generate_catena_device_DEVICE_MODEL_JSON})
    set(_TARGET ${generate_catena_device_TARGET})
    set(_OUT_DIR ${generate_catena_device_OUT_DIR})
    set(_HDR_OUT_VAR ${generate_catena_device_HDR_OUT_VAR})
    set(_SRC_OUT_VAR ${generate_catena_device_SRC_OUT_VAR})

    if(NOT DEFINED _TARGET AND (NOT DEFINED _HDR_OUT_VAR OR NOT DEFINED _SRC_OUT_VAR))
        message(SEND_ERROR "Error: generate_catena_device called without any targets or output variables")
        return()
    endif()

    if(NOT DEFINED _DEVICE_MODEL_JSON)
        message(SEND_ERROR "Error: generate_catena_device called without a device model json file")
        return()
    endif()

    if(NOT DEFINED _OUT_DIR)
        # default output directory to current binary directory
        set(_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    get_filename_component(device_name ${_DEVICE_MODEL_JSON} NAME)

    set(HEADER ${device_name}.h)
    set(BODY ${device_name}.cpp)

    # convert json to cpp
    find_program(NODE node REQUIRED)
    add_custom_command(
        OUTPUT ${_OUT_DIR}/${HEADER}
            ${_OUT_DIR}/${BODY}
        COMMAND ${NODE} ${CATENA_CODEGEN} --schema ${CATENA_SCHEMA} --device-model "${_DEVICE_MODEL_JSON}" --output ${_OUT_DIR}
        DEPENDS ${_DEVICE_MODEL_JSON} ${CATENA_SCHEMA} ${CATENA_CODEGEN}
        COMMENT "Generating ${HEADER} and ${BODY} from ${_DEVICE_MODEL_JSON}"
    )

    set_source_files_properties(${_OUT_DIR}/${HEADER} ${_OUT_DIR}/${BODY} PROPERTIES GENERATED TRUE)

    if(DEFINED _TARGET)
        target_sources(${TARGET} PRIVATE ${_OUT_DIR}/${BODY})
        target_include_directories(${TARGET} PRIVATE ${_OUT_DIR})
    endif()

    if(DEFINED _HDR_OUT_VAR)
        set(${generate_catena_device_HDR_OUT_VAR} ${_OUT_DIR}/${HEADER} PARENT_SCOPE)
    endif()
    if(DEFINED _SRC_OUT_VAR)
        set(${generate_catena_device_SRC_OUT_VAR} ${_OUT_DIR}/${BODY} PARENT_SCOPE)
    endif()

endfunction()