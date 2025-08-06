FROM ubuntu:24.04

ARG EXAMPLE
ARG WORKDIR

RUN apt-get update && apt-get install -y --no-install-recommends \
    libgoogle-glog-dev \
    libboost-url1.83.0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR ${WORKDIR}

COPY ${EXAMPLE} ${WORKDIR}/

RUN mkdir -p ${WORKDIR}/logs