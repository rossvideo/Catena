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

generate_catena_device(
    DEVICE_MODEL_JSON ${CMAKE_CURRENT_SOURCE_DIR}/device.${TARGET}.json
    TARGET ${TARGET}
)

target_sources(${TARGET}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.cpp     
)

target_link_libraries(${TARGET}
    Catena_cpp::catena_connections_grpc
    absl::flags_parse
)

target_compile_features(${TARGET}
    PUBLIC
        cxx_std_20
)



