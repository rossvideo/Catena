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
#
# cmake build file for C++ SDKs and examples.
#
#

cmake_minimum_required(VERSION 3.20)

project(COMMON C CXX)

set(target catena_common)

set(sources 
    "src/utils.cpp" 
    "src/vdk/signals.cpp" 
    "src/Path.cpp"
    "src/Authorization.cpp"
    "src/Device.cpp"
    "src/ParamDescriptor.cpp"
    "src/StructInfo.cpp"
    "src/PolyglotText.cpp"
    "src/LanguagePack.cpp"
    "src/Menu.cpp"
    "src/MenuGroup.cpp"
    "src/ParamVisitor.cpp"
    "src/SubscriptionManager.cpp"
    "src/SharedFlags.cpp"
    "src/ConnectionQueue.cpp"
)

# conditionally make for gRPC
if (gRPC_enabled)
    include(gRPC.cmake)
    cmake_catena_grpc_common()
endif(gRPC_enabled)

# conditionally make for REST
if (REST_enabled)
    include(proto_interface.cmake)
    cmake_catena_proto_common()
endif(REST_enabled)

# make the examples
add_subdirectory(examples)
