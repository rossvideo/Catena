# SINGLE_SOURCE is used to specify the source image for the single example.
# It is required to be set when building a single example image.
# If not set, the build will fail. Should be either 'rest' or 'grpc'.
# If you want to build a combined image, target 'all' instead.
ARG SINGLE_SOURCE=required

FROM ubuntu:24.04 AS base

ARG CONNECTION
ARG EXAMPLE
ARG EXAMPLE_PATH
ARG WORKDIR
ARG USER_GROUP=1000:1000

ENV CATENA_CONNECTION=${CONNECTION}
ENV CATENA_EXAMPLE=${EXAMPLE}

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    wget \
    libgoogle-glog-dev \
    libboost-url1.83.0 \
    && rm -rf /var/lib/apt/lists/* \
    && wget https://github.com/fullstorydev/grpcurl/releases/download/v1.9.3/grpcurl_1.9.3_linux_amd64.deb \
    && dpkg -i grpcurl_1.9.3_linux_amd64.deb \
    && rm grpcurl_1.9.3_linux_amd64.deb

USER ${USER_GROUP}

WORKDIR ${WORKDIR}

COPY --chmod=755 --chown=${USER_GROUP} interface ${WORKDIR}/st2138
COPY --chmod=755 docker/example-entrypoint.sh /entrypoint.sh
COPY --chmod=755 docker/example-healthcheck.sh /healthcheck.sh

HEALTHCHECK --interval=30s --timeout=5s --retries=3 CMD /healthcheck.sh

RUN mkdir -p ${WORKDIR}/logs && mkdir -p ${WORKDIR}/static

ENTRYPOINT ["/bin/sh", "/entrypoint.sh"]

FROM base AS rest

ENV CATENA_PORT=443

FROM base AS grpc

ENV CATENA_PORT=6254

FROM ${SINGLE_SOURCE} AS single

ARG CONNECTION
ARG EXAMPLE
ARG EXAMPLE_PATH
ARG WORKDIR
ARG USER_GROUP

COPY --chmod=755 --chown=${USER_GROUP} ${EXAMPLE_PATH} ${WORKDIR}/${CONNECTION}/${EXAMPLE}

FROM grpc AS all

ARG EXAMPLE_PATH
ARG WORKDIR
ARG USER_GROUP

COPY --chmod=755 --chown=${USER_GROUP} ${EXAMPLE_PATH} ${WORKDIR}
