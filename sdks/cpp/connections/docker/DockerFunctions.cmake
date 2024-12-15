#
# Copyright 2024 Ross Video Ltd
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
# LIABLE FOR ANY DIRECT, 
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#
# functions used by the local CMakeLists.txt
#

# function to make an image using a custom command
# Arguments:
# . stem: the base name to use this will be used to locate the appropriate
#   Dockerfile and name the output artifacts
# . context: path to the source files to COPY into the image
# . dependencies: list, possibly empty of targets on which the image depends
#
# Note that, because the output is a Docker image which cmake CANNOT
# track as an output, the custom command also writes an output file
# containing the hash of the image created so dependencies can be 
# chained to it
#
function (make_image stem context dependencies)
  # name the artifacts after the stem
  set(image_name "${stem}")
  set(dockerfile "${CMAKE_CURRENT_SOURCE_DIR}/${stem}_dockerfile")
  set(image_hash "${stem}.hash")

  message(STATUS "image: ${image_name}, dockerfile: ${dockerfile}, hash: ${image_hash}")

  set(_dependencies ${dockerfile})
  if(dependencies)
    list(APPEND _dependencies ${dependencies})
  endif(dependencies)
  
  # suppress deprecation notices about docker build
  set($ENV{DOCKER_BUILDKIT} 0)
  
  # custom command to make docker images
  add_custom_command(
    OUTPUT ${image_hash}
    COMMENT "Building Docker Image \"${image_name}\""
    
    # build the docker image
    COMMAND ${DOCKER_CMD} build
        --tag ${image_name}
        --file ${dockerfile}
        ${context}

    # output the hash as a build artifact
    COMMAND ${DOCKER_CMD} inspect --format='{{index .Id}}' ${image_name} > ${image_hash}

    DEPENDS ${_dependencies}
  )
  add_custom_target(${image_name} ALL
    DEPENDS ${image_hash}
  )

endfunction ()