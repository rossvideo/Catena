#!/bin/bash

SCRIPT_DIR="$(readlink -f "$(dirname -- "${BASH_SOURCE[0]}")")"
. "${SCRIPT_DIR}/docker/opt/common.sh"

print_logo_anim 2 0.01
set -ex

IMAGE_TAG="${IMAGE_TAG:-latest}"
IMAGE_NAME="${IMAGE_NAME:-nex-vs-cicd-automation/mcm/catena-sdk-builder}"
IMAGE_REGISTRY="${IMAGE_REGISTRY:-docker.io}"
IMAGE_CACHE_REGISTRY="${IMAGE_CACHE_REGISTRY:-docker.io}"


docker buildx build \
        --network=host \
        --build-arg="https_proxy=${https_proxy}" \
        --build-arg="http_proxy=${http_proxy}" \
        --build-arg="IMAGE_CACHE_REGISTRY=${IMAGE_CACHE_REGISTRY}" \
        -t "${IMAGE_REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG}" \
        -f "${SCRIPT_DIR}/docker/builder.dockerfile" \
        "${SCRIPT_DIR}"

docker buildx build \
        --network=host \
        --build-arg="https_proxy=${https_proxy}" \
        --build-arg="http_proxy=${http_proxy}" \
        --build-arg="BUILDER_TAG=${IMAGE_TAG}" \
        --build-arg="BUILDER_NAME=${IMAGE_NAME}" \
        --build-arg="BUILDER_REGISTRY=${IMAGE_REGISTRY}" \
        --build-arg="IMAGE_CACHE_REGISTRY=${IMAGE_CACHE_REGISTRY}" \
        -t "${IMAGE_REGISTRY}/nex-vs-cicd-automation/mcm/catena-sdk:${IMAGE_TAG}" \
        -f "${SCRIPT_DIR}/docker/runner.dockerfile" \
        "${SCRIPT_DIR}"
