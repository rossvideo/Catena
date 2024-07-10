#!/bin/bash

sdk_path=~/Catena/sdks/cpp

if [ ! -d $sdk_path ]; then
    echo "Error Catena sdk not properly not mounted."
    exit 1
fi

rm -r ${sdk_path}/build
mkdir -p ${sdk_path}/build
cd ${sdk_path}/build
cmake -G Ninja -DUNIT_TESTING=ON ..
ninja