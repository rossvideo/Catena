FROM ghcr.io/rossvideo/catena-go-toolchain:latest

# Declare build arguments
ARG USER_UID=1000
ARG USER_GID=1000
ARG USER_NAME=catenauser
ARG IMAGE_TITLE
ARG IMAGE_DESCRIPTION
ARG IMAGE_VERSION
ARG IMAGE_VENDOR
ARG BUILD_TYPE
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
ENV BUILD_TYPE=${BUILD_TYPE}
ENV CONNECTIONS=${CONNECTIONS}
ENV BUILD_TARGET=${BUILD_TARGET}
ENV GOPATH=/home/${USER_NAME}/go
ENV PATH=${PATH}:${GOPATH}/bin

USER root

RUN if getent passwd $USER_UID > /dev/null; then userdel $(getent passwd $USER_UID | cut -d: -f1); fi

RUN if getent group $USER_GID > /dev/null; then groupdel $(getent group $USER_GID | cut -d: -f1); fi

# Create user with specified UID and GID
RUN groupadd -g $USER_GID $USER_NAME && \
    useradd -u $USER_UID -g $USER_GID -m $USER_NAME

# Add user to sudoers
RUN echo "${USER_NAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

RUN sudo apt-get update && sudo apt-get install -y protobuf-compiler

# Set the working directory
WORKDIR /home/${USER_NAME}/Catena

USER ${USER_NAME}

RUN go install golang.org/x/tools/gopls@latest \
    && go install github.com/go-delve/delve/cmd/dlv@latest \
    && go install google.golang.org/protobuf/cmd/protoc-gen-go@latest \
    && go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest

RUN export PATH="$PATH:$(go env GOPATH)/bin"
ENTRYPOINT ["/bin/sh", "-c", "/bin/bash"]
