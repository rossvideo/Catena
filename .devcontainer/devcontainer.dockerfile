FROM ghcr.io/rossvideo/catena-toolchain-cpp:latest

#FROM ghcr.io/rossvideo/catena-toolchain-cpp:latest

# Use environment variables for labels
LABEL org.opencontainers.image.title=$IMAGE_TITLE
LABEL org.opencontainers.image.description=$IMAGE_DESCRIPTION
LABEL org.opencontainers.image.version=$IMAGE_VERSION
LABEL org.opencontainers.image.vendor=$IMAGE_VENDOR

# Clone Catena repository
RUN mkdir -p ~/Catena \
    && cd ~/Catena \
    && git init \
    && git remote add origin https://github.com/rossvideo/Catena.git \
    && git pull origin develop \
    && git submodule update --init --recursive

# Build Catena
RUN mkdir -p ~/Catena/sdks/cpp/build \
    && cd ~/Catena/sdks/cpp/build \
    && cmake -G Ninja -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DCONNECTIONS=$CONNECTIONS -DCMAKE_INSTALL_PREFIX=~/.local ~/Catena/sdks/cpp \
    && ninja

# Set the default user
WORKDIR "/root/Catena"

# ~/Catena/sdks/cpp/build/connections/gRPC/examples/status_update/status_update --static_root ../connections/gRPC/examples/status_update/static
# #docker run -rm -v $(realpath ../):~/Catena -it catena-sdk
VOLUME ["/root/Catena"] 


#healthcheck
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD ~/Catena/sdks/cpp/build#common/examples/hello_world/hello_world || exit 1
