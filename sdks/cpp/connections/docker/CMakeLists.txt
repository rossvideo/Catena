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
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

#
# This cmake file is used to build some example services as docker images for
# use in grpc_compose.yml.
#

cmake_minimum_required(VERSION 3.20)
project(catena-docker-services)

# Ensure we have the docker app, alias it to DOCKER_CMD.
find_program(DOCKER_CMD docker REQUIRED)

# Include the make_image function.
include(DockerFunctions.cmake)

# Make our base image.
make_image(base . "")

# Make the status update image.
make_image(catena_status_update ${CATENA_CPP_ROOT_DIR} "base.hash;status_update")

# Make the use menus image.
make_image(catena_use_menus ${CATENA_CPP_ROOT_DIR} "base.hash;use_menus")

# Make the structs with authz image.
make_image(catena_structs_with_authz ${CATENA_CPP_ROOT_DIR} "base.hash;structs_with_authz")

# Make the structs with authz image.
make_image(catena_use_commands ${CATENA_CPP_ROOT_DIR} "base.hash;use_commands")

# Pulls Envoy Proxy image v.1.33.
execute_process (
    COMMAND ${DOCKER_CMD} pull envoyproxy/envoy:v1.33-latest
    RESULT_VARIABLE result
)
if (NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to pull the envoy image")
endif()

# Make envoy_proxy image with entrypoint envoy/envoy_entrypoint.sh.
make_image(catena_envoy ${CATENA_CPP_ROOT_DIR} "")
