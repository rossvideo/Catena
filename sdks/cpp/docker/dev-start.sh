#!/bin/bash

# file: dev-start.sh
# description: Script to pull and build the catena-toolchain docker image with required configurations.
# copyright: Copyright (c) 2024 Ross Video
# author: isaac.robert@rossvideo.com

if [ -z "$1" ]; then
    echo "Usage: $0 <absolute/path/to/Catena>"
    exit 1
fi

docker_gid=$(cut -d: -f3 < <(getent group docker))

docker buildx build \
    -f $1/sdks/cpp/docker/dockerfiles/Dockerfile.develop \
    -t catena-develop:latest \
    --build-arg USER_UID=$(id -u) \
    --build-arg USER_GID=$docker_gid \
    --build-arg USERNAME=$USER \
    --build-arg GROUPNAME="docker" \
    $1

docker run \
    -it \
    -v $1:/home/${USER}/Catena \
    -w /home/${USER}/Catena/sdks/cpp \
    -p 6254:6254 \
    --rm \
    catena-develop:latest \
    /bin/bash -c "~/Catena/sdks/cpp/docker/build.sh; /bin/bash"