#!/bin/bash

SCRIPT_DIR="$(readlink -f "$(dirname -- "${BASH_SOURCE[0]}")")"
. "${SCRIPT_DIR}/docker/opt/common.sh"

print_logo_anim 2 0.01
set -ex

IMAGE_REGISTRY="${IMAGE_REGISTRY:-docker.io}"
IMAGE_NAME="${IMAGE_NAME:-ubuntu-2204/catena-sdk}"
IMAGE_TAG="${IMAGE_TAG:-latest}"

docker buildx build --network=host \
        --build-arg="https_proxy=${https_proxy}" \
        --build-arg="http_proxy=${http_proxy}" \
        -t "${IMAGE_REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG}" \
        -f "${SCRIPT_DIR}/docker/ubuntu-2204.dockerfile" \
        "${SCRIPT_DIR}"
