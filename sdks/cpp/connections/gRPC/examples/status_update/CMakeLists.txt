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



get_filename_component(TARGET ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${TARGET} C CXX)

add_executable(${TARGET})

# set up some directory names
set(HEADER device.${TARGET}.json.h)
set(BODY device.${TARGET}.json.cpp)
get_filename_component(BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} ABSOLUTE)
set(DEVICE_MODEL_JSON ${CMAKE_CURRENT_SOURCE_DIR}/device.${TARGET}.json)
set(TOOLS_DIR ${CATENA_ROOT_DIR}/tools)
set(CODEGEN ${TOOLS_DIR}/codegen/index.js)
set(SCHEMA_DIR ${CATENA_ROOT_DIR}/schema)
set(SCHEMA ${SCHEMA_DIR}/catena.schema.json)
set(CODEGEN_OUTPUT ${HEADER} ${BODY})


# convert json to cpp
set(HEADER device.${TARGET}.json.h)
set(BODY device.${TARGET}.json.cpp)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${HEADER}
           ${CMAKE_CURRENT_BINARY_DIR}/${BODY}
    COMMAND node ${CODEGEN} --schema ${SCHEMA} --device-model "${DEVICE_MODEL_JSON}" --output ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${DEVICE_MODEL_JSON} ${SCHEMA} ${CODEGEN}
    COMMENT "Generating ${HEADER} and ${BODY} from ${DEVICE_MODEL_JSON} and ${SCHEMA} using ${CODEGEN}"
)



# Step 2: Create a custom target that depends on the output of the custom command
add_custom_target(${TARGET}_codegen
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${HEADER} ${CMAKE_CURRENT_BINARY_DIR}/${BODY}
)

set_source_files_properties(${CODEGEN_OUTPUT} PROPERTIES GENERATED TRUE)
target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CATENA_CPP_ROOT_DIR}>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_PREFIX_PATH}>
        $<BUILD_INTERFACE:${PROTOBUF_INCLUDE_DIRS}>
)

target_sources(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/${BODY}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.cpp>      
)

add_dependencies(${TARGET} ${TARGET}_codegen)

target_link_libraries(${TARGET}
    catena_connections_grpc
    absl::flags_parse
)

target_compile_features(${TARGET}
    PUBLIC
        cxx_std_20
)




