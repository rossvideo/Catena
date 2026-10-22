FROM library/ubuntu:24.04 AS base

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
    protobuf-compiler=${PROTOBUF_COMPILER_VERSION} \
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
COPY smpte/.nvmrc /root/smpte/.nvmrc
COPY smpte/scripts/common.sh /root/smpte/scripts/common.sh
WORKDIR /root/smpte
RUN IN_CONTAINER_BUILD=1 YQ_INSTALL_DIR=/usr/local/bin ./install-tooling.sh

WORKDIR /toolchain

COPY .devcontainer/go/go.mod .devcontainer/go/go.sum ./

# Install the Go version required by the toolchain's go.mod (the toolchain line wins over the go line)
RUN GO_VERSION=$(awk '/^toolchain /{t=$2} /^go /{g=$2} END{ if (t == "") { if (split(g, a, ".") == 2) g = g ".0"; t = "go" g } print t }' go.mod).linux-amd64 \
    && echo "Installing Go version ${GO_VERSION}" \
    && wget https://go.dev/dl/${GO_VERSION}.tar.gz \
    && sudo tar -C /usr/local -xzf ${GO_VERSION}.tar.gz \
    && rm ${GO_VERSION}.tar.gz

ENV GOBIN=/usr/local/bin
ENV PATH=$PATH:/usr/local/go/bin
# Fail the build rather than auto-download a toolchain under /root
ENV GOTOOLCHAIN=local

RUN go mod download

# will install all the tools from the toolchain's go.mod
RUN go install tool

ENV GOBIN=
ENV GOTOOLCHAIN=

USER ubuntu
WORKDIR /home/ubuntu
