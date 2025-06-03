# catena-sdk

ARG BUILDER_TAG="latest"
ARG BUILDER_NAME="nex-vs-cicd-automation/mcm/catena-sdk-builder"
ARG BUILDER_REGISTRY="docker.io"
ARG IMAGE_TAG=22.04@sha256:81bba8d1dde7fc1883b6e95cd46d6c9f4874374f2b360c8db82620b33f6b5ca1
ARG IMAGE_NAME=library/ubuntu
ARG IMAGE_CACHE_REGISTRY=docker.io

FROM ${BUILDER_REGISTRY}/${BUILDER_NAME}:${BUILDER_TAG} as builder

ARG IMAGE_TAG
ARG IMAGE_NAME
ARG IMAGE_CACHE_REGISTRY

FROM ${IMAGE_CACHE_REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG}

LABEL org.opencontainers.image.title="Catena SDK"
LABEL org.opencontainers.image.description="Rossvideo Catena and Intel NEX visual solutions collaboration"
LABEL org.opencontainers.image.version="v0.1.0"
LABEL org.opencontainers.image.vendor="Rossvideo"

ENV DEBIAN_FRONTEND=noninteractive
ARG PREFIX_DIR="/usr/local"

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
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/${BUILD_TARGET}/common/examples/full_service/full_service /opt/catena-sdk/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/${BUILD_TARGET}/common/examples/serdes/serdes /opt/catena-sdk/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/${BUILD_TARGET}/common/examples/basic_param_access/basic_param_access /opt/catena-sdk/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/${BUILD_TARGET}/common/tests/ParamAccessorTest /opt/catena-sdk/tests/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/${BUILD_TARGET}/common/libcatena_common.a /opt/catena-sdk/lib/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/${BUILD_TARGET}/libcatena_interface.a /opt/catena-sdk/lib/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/example_device_models /opt/catena-sdk/example_device_models/
COPY --chown=1000:1000 --from=builder /opt/catena-sdk/interface /opt/catena-sdk/interface/

RUN ldconfig

COPY docker/opt /opt

ENTRYPOINT [ "/opt/entrypoint.sh" ]
