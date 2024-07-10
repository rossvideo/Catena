#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 <path/to/Catena>"
    exit 1
fi

docker_gid=$(cut -d: -f3 < <(getent group docker))

docker buildx build \
    -f $1/sdks/cpp/docker/dockerfiles/Dockerfile.develop \
    -t catena-develop:v0.1.0 \
    --build-arg USER_UID=$(id -u) \
    --build-arg USER_GID=$docker_gid \
    --build-arg USERNAME=$USER \
    --build-arg GROUPNAME="docker" \
    $1

docker run \
    -it \
    -v $1:/home/${USER}/Catena \
    -w /home/${USER}/Catena/sdks/cpp \
    --rm \
    catena-develop:v0.1.0 \
    /bin/bash -c "$1/sdks/cpp/docker/build.sh; /bin/bash"