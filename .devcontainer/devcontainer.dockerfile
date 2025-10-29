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
ENV CMAKE_COMMAND="cmake -G Ninja -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCONNECTIONS=${CONNECTIONS} -DCOVERAGE=ON -DCMAKE_INSTALL_PREFIX=/usr/local/.local -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -DLOG_DIR=~/Catena/logs -B ~/Catena/${BUILD_TARGET} -S ~/Catena/sdks/cpp"

USER root

RUN if getent passwd $USER_UID > /dev/null; then userdel $(getent passwd $USER_UID | cut -d: -f1); fi

RUN if getent group $USER_GID > /dev/null; then groupdel $(getent group $USER_GID | cut -d: -f1); fi

# Create user with specified UID and GID
RUN groupadd -g $USER_GID $USER_NAME && \
    useradd -u $USER_UID -g $USER_GID -m $USER_NAME

# Add user to sudoers
RUN echo "${USER_NAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers




# Install Docker & Docker Compose

# remove the docker group if it exists
# RUN if getent group docker; then sudo groupdel docker; fi

# enable docker
# RUN if ! getent group docker; then sudo groupadd docker; fi \
#     && sudo usermod -aG docker $USER_NAME \
#     && sudo systemctl enable docker.service \
#     && sudo systemctl enable containerd.service \
#     && sudo service docker start

# Set the working directory
WORKDIR /home/${USER_NAME}/Catena

USER ${USER_NAME}
ENTRYPOINT ["/bin/sh", "-c", "/bin/bash"]

# nothing yet, but this is where we would add any additional devcontainer-specific setup
# such as installing VS Code extensions or additional tools