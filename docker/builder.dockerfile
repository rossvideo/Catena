# catena-sdk-builder

ARG IMAGE_TAG=22.04@sha256:81bba8d1dde7fc1883b6e95cd46d6c9f4874374f2b360c8db82620b33f6b5ca1
ARG IMAGE_NAME=library/ubuntu
ARG IMAGE_CACHE_REGISTRY=docker.io

FROM ${IMAGE_CACHE_REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG}

ARG http_proxy
ARG https_proxy
ARG no_proxy
ARG NPROC

ARG PREFIX_DIR="/usr/local"
ARG GPRC_VERSION="v1.58.0"
ARG JWT_CPP_DIR="/opt/jwt-cpp"
ARG GRPC_DIR="/opt/grpc"
ARG CATENA_SDK="/opt/catena-sdk"

ENV DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-o", "pipefail", "-exc"]
RUN apt-get update --fix-missing && \
    apt-get -y full-upgrade && \
    apt-get install -y --no-install-recommends \
        git git-lfs \
        curl wget \
        tar unzip \
        ca-certificates \
        build-essential \
        libssl-dev \
        libgtest-dev \
        cmake make \
        jq  doxygen \
        graphviz && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# jwt-cpp required dependency
WORKDIR "${JWT_CPP_DIR}"
RUN git clone https://github.com/Thalhammer/jwt-cpp "${JWT_CPP_DIR}" && \
    mkdir -p "${JWT_CPP_DIR}/build" && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${PREFIX_DIR}" \
        -S "${JWT_CPP_DIR}" \
        -B "${JWT_CPP_DIR}/build" && \
    make -C "${JWT_CPP_DIR}/build" -j "${NPROC:-$(nproc)}" && \
    make -C "${JWT_CPP_DIR}/build" install -j "${NPROC:-$(nproc)}"

# grpc required dependency
WORKDIR "${GRPC_DIR}"
RUN git clone --recurse-submodules -b ${GPRC_VERSION} --depth 1 --shallow-submodules https://github.com/grpc/grpc "${GRPC_DIR}" && \
    mkdir -p "${GRPC_DIR}/cmake/build" && \
    cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=ON -DCMAKE_INSTALL_PREFIX="${PREFIX_DIR}" \
        -S "${GRPC_DIR}" \
        -B "${GRPC_DIR}/cmake/build" && \
    make -C "${GRPC_DIR}/cmake/build" -j "${NPROC:-$(nproc)}" && \
    make -C "${GRPC_DIR}/cmake/build" install && \
    make -C "${GRPC_DIR}/cmake/build" -j "${NPROC:-$(nproc)}" grpc_cli && \
    cp "${GRPC_DIR}/cmake/build/grpc_cli" "${PREFIX_DIR}/bin/" && \
    rm -rf "${GRPC_DIR}"

# catena-sdk
COPY . "${CATENA_SDK}/"

WORKDIR "${CATENA_SDK}/sdks/cpp/build"
RUN cmake -S "${CATENA_SDK}/sdks/cpp" -B "${CATENA_SDK}/sdks/cpp/build" && \
    make -C "${CATENA_SDK}/sdks/cpp/build" -j "${NPROC:-$(nproc)}"
