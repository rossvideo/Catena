FROM ghcr.io/rossvideo/catena-toolchain-cpp:latest AS builder


# Clone Catena repository
RUN mkdir -p /root/Catena \
    && cd /root/Catena \
    && git init \
    && git remote add origin https://github.com/rossvideo/Catena.git \
    && git pull origin develop \
    && git submodule update --init --recursive

# Build Catena
RUN mkdir -p /root/Catena/sdks/cpp/build \
    && cd /root/Catena/sdks/cpp/build \
    && cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCONNECTIONS='gRPC;REST' -DCMAKE_INSTALL_PREFIX=/usr/local/.local /root/Catena/sdks/cpp \
    && ninja


FROM library/ubuntu:24.04

LABEL org.opencontainers.image.source="https://github.com/rossvideo/Catena"
LABEL org.opencontainers.image.title="Catena"
LABEL org.opencontainers.image.description="compiled catena from dev branch"
LABEL org.opencontainers.image.version="0.1.0"
LABEL org.opencontainers.image.vendor="Ross Video"

COPY --from=builder /root/Catena/sdks/cpp/build/ /usr/local/Catena/
