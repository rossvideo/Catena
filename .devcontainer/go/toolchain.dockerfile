FROM library/ubuntu:24.04 AS base
LABEL org.opencontainers.image.title=$IMAGE_TITLE
LABEL org.opencontainers.image.description=$IMAGE_DESCRIPTION
LABEL org.opencontainers.image.version=$IMAGE_VERSION
LABEL org.opencontainers.image.vendor=$IMAGE_VENDOR

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Copy the toolchain.env file
COPY .devcontainer/go/toolchain.env /root/toolchain.env

# Source the toolchain.env file
RUN . /root/toolchain.env \
    && apt-get update \
    && apt-get install --no-install-recommends -y \
    build-essential \
    sudo \
    git \
    openssh-client \
    libssl-dev=$OPENSSL_VERSION \
    doxygen=$DOXYGEN_VERSION \
    valgrind=$VALGRIND_VERSION \
    curl \
    ca-certificates \
    wget \
    && curl -sL https://deb.nodesource.com/setup_${NODEJS_VERSION}.x -o /tmp/nodesource_setup.sh \
    && cat /tmp/nodesource_setup.sh \
    && bash /tmp/nodesource_setup.sh \
    && apt-get install -y nodejs \
    && rm -f /tmp/nodesource_setup.sh \
    && npm install -g n \
    && apt-get clean

# install Go
RUN . /root/toolchain.env \
    && wget https://go.dev/dl/${GO_VERSION}.tar.gz \
    && sudo tar -C /usr/local -xzf ${GO_VERSION}.tar.gz \
    && rm ${GO_VERSION}.tar.gz

ENV PATH="/usr/local/go/bin:${PATH}"

# Install Docker & Docker Compose
RUN . /root/toolchain.env \
    && apt-get update \
    && apt-get install --no-install-recommends -y ca-certificates curl \
    && install -m 0755 -d /etc/apt/keyrings \
    && curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc \
    && chmod a+r /etc/apt/keyrings/docker.asc \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}") stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null \
    && apt-get update \
    && apt-get install -y docker-ce-cli docker-buildx-plugin docker-compose-plugin

# Build OpenAPI
COPY smpte/install-tooling.sh /root/smpte/install-tooling.sh
WORKDIR /root/smpte
RUN ./install-tooling.sh

# Install protobuf compiler and Go tools
RUN . /root/toolchain.env \
    && apt-get update \
    && apt-get install --no-install-recommends -y protobuf-compiler=${PROTOBUF_COMPILER_VERSION} \
    && apt-get clean

RUN go install golang.org/x/tools/gopls@v0.21.1 \
    && go install github.com/go-delve/delve/cmd/dlv@v1.26.2 \
    && go install google.golang.org/protobuf/cmd/protoc-gen-go@v1.36.11 \
    && go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@v1.6.2 \
    && go install github.com/axw/gocov/gocov@v1.2.1 \
    && go install github.com/matm/gocov-html/cmd/gocov-html@v1.4.0 \
    && go install github.com/jandelgado/gcov2lcov@v1.1.1

ENV PATH="/root/go/bin:${PATH}"

USER ubuntu
WORKDIR /home/ubuntu
