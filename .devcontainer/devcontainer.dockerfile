FROM ghcr.io/rossvideo/catena-toolchain-cpp:latest

# Declare build arguments
ARG USER_UID=1000
ARG USER_GID=1000
ARG USER_NAME=catenauser
ARG IMAGE_TITLE
ARG IMAGE_DESCRIPTION
ARG IMAGE_VERSION
ARG IMAGE_VENDOR
ARG CMAKE_BUILD_TYPE
ARG CONNECTIONS
ARG BUILD_TARGET

# Use build arguments for labels
LABEL org.opencontainers.image.title=${IMAGE_TITLE}
LABEL org.opencontainers.image.description=${IMAGE_DESCRIPTION}
LABEL org.opencontainers.image.version=${IMAGE_VERSION}
LABEL org.opencontainers.image.vendor=${IMAGE_VENDOR}

ENV USER_UID=${USER_UID}
ENV USER_GID=${USER_GID}
ENV USER_NAME=${USER_NAME}
ENV CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
ENV CONNECTIONS=${CONNECTIONS}
ENV BUILD_TARGET=${BUILD_TARGET}

RUN if getent passwd $USER_UID > /dev/null; then userdel $(getent passwd $USER_UID | cut -d: -f1); fi

RUN if getent group $USER_GID > /dev/null; then groupdel $(getent group $USER_GID | cut -d: -f1); fi

# Create user with specified UID and GID
RUN groupadd -g $USER_GID $USER_NAME && \
    useradd -u $USER_UID -g $USER_GID -m $USER_NAME

# Add user to sudoers
RUN echo "${USER_NAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers


USER ${USER_NAME}

# Install Docker & Docker Compose
RUN sudo apt-get update

# remove the docker group if it exists
# RUN if getent group docker; then sudo groupdel docker; fi

# enable docker
# RUN if ! getent group docker; then sudo groupadd docker; fi \
#     && sudo usermod -aG docker $USER_NAME \
#     && sudo systemctl enable docker.service \
#     && sudo systemctl enable containerd.service \
#     && sudo service docker start


# Clone Catena repository
RUN mkdir -p ~/Catena \
    && cd ~/Catena \
    && git init \
    && git remote add origin https://github.com/rossvideo/Catena.git \
    && git pull origin develop \
    && git submodule update --init --recursive

# Build Catena
RUN mkdir -p ~/Catena/${BUILD_TARGET} \
    && cd ~/Catena/${BUILD_TARGET} \
    && cmake -G Ninja -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DCONNECTIONS=$CONNECTIONS -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_C_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage"  -DCMAKE_INSTALL_PREFIX=/usr/local/.local -B ~/Catena/${BUILD_TARGET} ~/Catena/sdks/cpp  \
    && ninja
# cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCONNECTIONS='gRPC;REST' -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_C_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage"  -DCMAKE_INSTALL_PREFIX=/usr/local/.local ~/Catena/sdks/cpp
RUN cd ~/Catena/ \
    && ./scripts/run_coverage.sh --html --verbose
# Set the working directory
WORKDIR /home/${USER_NAME}/Catena

# ~/Catena/${BUILD_TARGET}/connections/gRPC/examples/status_update/status_update --static_root ../connections/gRPC/examples/status_update/static
# #docker run -rm -v $(realpath ../):~/Catena -it catena-sdk
VOLUME ["/home/${USER_NAME}/Catena"] 

ENTRYPOINT ["/bin/sh", "-c", "/bin/bash"]
# Healthcheck
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD ~/Catena/${BUILD_TARGET}/common/examples/hello_world/hello_world || exit 1
