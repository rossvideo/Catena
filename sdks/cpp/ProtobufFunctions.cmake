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
# Function definition for protobuf preprocessor
#
#

cmake_minimum_required(VERSION 3.20)

# Function to preprocess protobuf files
# Arguments: 
# . catena_interface_dir location of the protobuf interface definition files
# . proto_stems list of the names of the proto files to preprocess
# . proto_target the target that the preprocessed files will be added to
# . model the runtime model to optimize for
#
# Returns:
#  ${model}_sources the list of filenames preprocessed
#
function(preprocess_protobuf_files catena_interface_dir proto_stems proto_target model)
    
    set(optimize_for CODE_SIZE)
    if (${model} STREQUAL "lite")
        set(optimize_for LITE_RUNTIME)
    endif()
    message(STATUS "Building Catena model for ${model} runtime optimizing for ${optimize_for}")

    foreach(_proto ${proto_stems})
        # preprocess proto files
        set(input "${catena_interface_dir}${_proto}.proto")
        set(output "${CMAKE_BINARY_DIR}/${model}/${_proto}.proto")
        
        add_custom_command(
            OUTPUT "${output}"
            COMMAND ${CPP} ${cpp_opts} -D OPTIMIZE_FOR=${optimize_for} -o "${output}" "${input}"
            DEPENDS "${input}"
            COMMENT "Preprocessing ${input} into ${output}"
            VERBATIM
        )

        set_source_files_properties(${output} PROPERTIES GENERATED TRUE)
        target_sources(${proto_target} PRIVATE ${output})

    endforeach()
endfunction()
