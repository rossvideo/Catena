FROM ubuntu:24.04 AS base

ARG CONNECTION
ARG EXAMPLE
ARG EXAMPLE_PATH
ARG WORKDIR
ARG USER_GROUP=1000:1000

ENV CATENA_CONNECTION=${CONNECTION}
ENV CATENA_EXAMPLE=${EXAMPLE}

RUN apt-get update && apt-get install -y --no-install-recommends \
    libgoogle-glog-dev \
    && rm -rf /var/lib/apt/lists/*

USER ${USER_GROUP}

WORKDIR ${WORKDIR}

COPY --chmod=755 --chown=${USER_GROUP} ${EXAMPLE_PATH} ${WORKDIR}/${CONNECTION}/${EXAMPLE}
COPY --chmod=755 docker/example-entrypoint.sh /entrypoint.sh

RUN mkdir -p ${WORKDIR}/logs && mkdir -p ${WORKDIR}/static

ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]

FROM base AS grpc

# extra config just for gRPC

FROM base AS rest

ENV CATENA_PORT=443

USER root

RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-url1.83.0 \
    wget \
    && rm -rf /var/lib/apt/lists/*

USER ${USER_GROUP}

HEALTHCHECK --interval=30s --timeout=5s --retries=3 CMD wget -q --spider http://localhost:${CATENA_PORT}/st2138-api/v1/health || exit 1