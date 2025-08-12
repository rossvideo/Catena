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
COPY --chmod=755 --chown=${USER_GROUP} interface ${WORKDIR}/interface
COPY --chmod=755 docker/example-entrypoint.sh /entrypoint.sh
COPY --chmod=755 docker/example-healthcheck.sh /healthcheck.sh

HEALTHCHECK --interval=30s --timeout=5s --retries=3 CMD /healthcheck.sh

RUN mkdir -p ${WORKDIR}/logs && mkdir -p ${WORKDIR}/static

ENTRYPOINT ["/bin/sh", "/entrypoint.sh"]

FROM base AS grpc

ENV CATENA_PORT=6254

USER root

RUN apt-get update && apt-get install -y --no-install-recommends \
    wget \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && wget https://github.com/fullstorydev/grpcurl/releases/download/v1.9.3/grpcurl_1.9.3_linux_amd64.deb \
    && dpkg -i grpcurl_1.9.3_linux_amd64.deb \
    && rm grpcurl_1.9.3_linux_amd64.deb \
    && apt-get remove -y wget ca-certificates \
    && apt-get autoremove -y \
    && rm -rf /var/lib/apt/lists/*

USER ${USER_GROUP}

FROM base AS rest

ENV CATENA_PORT=443

USER root

RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-url1.83.0 \
    wget \
    && rm -rf /var/lib/apt/lists/*

USER ${USER_GROUP}
