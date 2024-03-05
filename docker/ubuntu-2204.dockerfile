# catena-sdk

ARG IMAGE_NAME=library/ubuntu:22.04@sha256:81bba8d1dde7fc1883b6e95cd46d6c9f4874374f2b360c8db82620b33f6b5ca1
ARG IMAGE_CACHE_REGISTRY=docker.io

FROM ${IMAGE_CACHE_REGISTRY}/${IMAGE_NAME} as builder

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
        jq && \
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
COPY "sdks/cpp" "${CATENA_SDK}/sdks/cpp/"
COPY "interface" "${CATENA_SDK}/interface"
COPY "example_device_models" "${CATENA_SDK}/example_device_models"

RUN apt-get update && apt install -y doxygen graphviz
WORKDIR "${CATENA_SDK}/sdks/cpp/build"
RUN cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${PREFIX_DIR}" \
        -S "${CATENA_SDK}/sdks/cpp" \
        -B "${CATENA_SDK}/sdks/cpp/build" && \
    make -C "${CATENA_SDK}/sdks/cpp/build" -j "${NPROC:-$(nproc)}"


FROM ${IMAGE_CACHE_REGISTRY}/${IMAGE_NAME}

LABEL org.opencontainers.image.title="Catena SDK"
LABEL org.opencontainers.image.description="Rossvideo Catena and Intel NEX visual solutions collaboration"
LABEL org.opencontainers.image.version="v0.1.0"
LABEL org.opencontainers.image.vendor="Rossvideo"

ENV DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-o", "pipefail", "-exc"]
RUN apt-get update --fix-missing && \
    apt-get -y full-upgrade && \
    apt-get install -y --no-install-recommends \
        locales apt-transport-https apt-utils \
        software-properties-common net-tools vim sudo \
        openssl libssl3 ca-certificates \
        git git-lfs curl wget tar unzip \
        pciutils iproute2 jq && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

COPY --chown=1000:1000 --from=builder ${PREFIX_DIR} ${PREFIX_DIR}
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/sdks/cpp/common/include /opt/catena-sdk/include/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/sdks/cpp/build/common/examples/full_service/full_service /opt/catena-sdk/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/sdks/cpp/build/common/examples/serdes/serdes /opt/catena-sdk/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/sdks/cpp/build/common/examples/basic_param_access/basic_param_access /opt/catena-sdk/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/sdks/cpp/build/common/tests/ParamAccessorTest /opt/catena-sdk/tests/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/sdks/cpp/build/common/libcatena_common.a /opt/catena-sdk/lib/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/sdks/cpp/build/libcatena_interface.a /opt/catena-sdk/lib/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/example_device_models /opt/catena-sdk/example_device_models/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/interface /opt/catena-sdk/interface/

RUN ldconfig

COPY docker/opt /opt

ENTRYPOINT [ "/opt/entrypoint.sh" ]
