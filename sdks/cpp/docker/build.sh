#!/bin/bash

# file: build.sh
# description: Script used to build the Catena sdk once inside the docker container.
# copyright: Copyright (c) 2024 Ross Video
# author: isaac.robert@rossvideo.com

sdk_path=~/Catena/sdks/cpp

if [ ! -d $sdk_path ]; then
    echo "Error Catena sdk not properly not mounted."
    exit 1
fi

# Install node dependencies
cd ~/Catena/tools/codegen
npm install

# Build the sdk
rm -r ${sdk_path}/build
mkdir -p ${sdk_path}/build
cd ${sdk_path}/build
# Change the cmake flags here as needed
cmake -G Ninja -DCONNECTIONS=gRPC -DCATENA_MODELS=lite ..
ninja