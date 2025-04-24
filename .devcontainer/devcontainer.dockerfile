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

# enable docker
RUN sudo groupadd docker \
    && sudo usermod -aG docker $USER \
    && newgrp docker \
    && sudo systemctl enable docker.service \
    && sudo systemctl enable containerd.service \
    && sudo service docker start



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
    && cmake -G Ninja -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DCONNECTIONS=$CONNECTIONS -DCMAKE_INSTALL_PREFIX=/usr/local/.local ~/Catena/sdks/cpp \
    && ninja

# Set the working directory
WORKDIR /home/${USER_NAME}/Catena

# ~/Catena/sdks/cpp/build/connections/gRPC/examples/status_update/status_update --static_root ../connections/gRPC/examples/status_update/static
# #docker run -rm -v $(realpath ../):~/Catena -it catena-sdk
VOLUME ["/home/${USER_NAME}/Catena"] 

ENTRYPOINT ["/bin/sh", "-c", "/bin/bash"]
# Healthcheck
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD ~/Catena/sdks/cpp/build/common/examples/hello_world/hello_world || exit 1
