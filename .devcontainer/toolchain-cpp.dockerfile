FROM library/ubuntu:24.04 AS base
LABEL org.opencontainers.image.title=$IMAGE_TITLE
LABEL org.opencontainers.image.description=$IMAGE_DESCRIPTION
LABEL org.opencontainers.image.version=$IMAGE_VERSION
LABEL org.opencontainers.image.vendor=$IMAGE_VENDOR

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Copy the toolchain.env file
COPY .devcontainer/toolchain-cpp.env /root/toolchain.env

# Source the toolchain.env file
RUN . /root/toolchain.env \
    && apt-get update \
    && apt-get install --no-install-recommends -y \
    build-essential \
    sudo \
    cmake=$CMAKE_VERSION \
    git \
    libssl-dev=$OPENSSL_VERSION \
    doxygen=$DOXYGEN_VERSION \
    graphviz=$GRAPHVIZ_VERSION \
    libgtest-dev=$GTEST_VERSION \
    libboost-all-dev=$BOOST_VERSION \
    libasio-dev=$ASIO_VERSION \
    libgmock-dev=$GMOCK_VERSION \
    ninja-build=$NINJA_VERSION \
    gdb=$GNU_DEBUGGER_VERSION \
    valgrind=$VALGRIND_VERSION \
    curl=$CURL_VERSION \
    libcurl4-openssl-dev=$LIBCURL_VERSION \
    libavahi-client-dev=$AVAHI_CLIENT_VERSION \
    libavahi-common-dev=$AVAHI_COMMON_VERSION \
    pkg-config=$PKG_CONFIG_VERSION \
    ca-certificates \
    && curl -sL https://deb.nodesource.com/setup_${NODEJS_VERSION}.x -o /tmp/nodesource_setup.sh \
    && cat /tmp/nodesource_setup.sh \
    && bash /tmp/nodesource_setup.sh \
    && apt-get install -y nodejs \
    && rm -f /tmp/nodesource_setup.sh \
    && npm install -g n \
    && apt-get clean \
    && rm -rf /var/cache/apt/archives/ /var/lib/apt/lists/*

# install lcov without relying on apt since the version in the ubuntu repos is outdated
RUN . /root/toolchain.env \
    && curl -L --output /tmp/lcov-$LCOV_VERSION.tar.gz https://github.com/linux-test-project/lcov/releases/download/v$LCOV_VERSION/lcov-$LCOV_VERSION.tar.gz \
    && tar -xzf /tmp/lcov-$LCOV_VERSION.tar.gz -C /tmp/ \
    && make -C /tmp/lcov-$LCOV_VERSION install \
    && rm -rf /tmp/lcov-$LCOV_VERSION /tmp/lcov-$LCOV_VERSION.tar.gz \
    && PERL_MM_USE_DEFAULT=1 cpan install App::cpanminus \
    && cpanm Capture::Tiny DateTime Date::Parse \
    && rm -rf /root/.cpanm /root/.cpan \
    && lcov --version

FROM base AS builder

# Install gRPC
RUN . /root/toolchain.env \
    && git clone -b $GRPC_VERSION https://github.com/grpc/grpc /usr/local/grpc \
    && git -C /usr/local/grpc submodule update --recursive --init \
    && git -C /usr/local/grpc/third_party/abseil-cpp checkout $ABSEIL_VERSION \
    && mkdir -p /usr/local/grpc/cmake/build

WORKDIR /usr/local/grpc/cmake/build
RUN . /root/toolchain.env \
    && cmake /usr/local/grpc -DCMAKE_BUILD_TYPE=Release -DgRPC_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/usr/local/.local \
    && make -j4 install

# Install jwt-cpp
RUN . /root/toolchain.env \
    && git clone -b $JWT_CPP_VERSION https://github.com/Thalhammer/jwt-cpp /usr/local/jwt-cpp \
    && mkdir -p /usr/local/jwt-cpp/build

WORKDIR /usr/local/jwt-cpp/build
RUN . /root/toolchain.env \
    && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local/.local /usr/local/jwt-cpp \
    && make && make install

# Install Google Test
WORKDIR /usr/src/gtest
RUN cmake -DCMAKE_INSTALL_PREFIX=/usr/local/.local CMakeLists.txt \
    && make

FROM base AS final

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

COPY .devcontainer/toolchain-cpp.requirements.txt /root/requirements.txt

# Install Python and gcovr for coverage reports
RUN apt-get update && apt-get install --no-install-recommends -y python3-pip \
    && pip3 install --no-cache-dir -r /root/requirements.txt --break-system-packages \
    && apt-get clean \
    && rm -rf /var/cache/apt/archives/ /var/lib/apt/lists/*

# Build OpenAPI
COPY smpte/install-tooling.sh /root/smpte/install-tooling.sh
WORKDIR /root/smpte
RUN ./install-tooling.sh

# this is the dir our builder stage CMAKE_INSTALL_PREFIX is set to
COPY --from=builder /usr/local/.local /usr/local/.local
# gtest doesn't have a make install target, so we have to copy the files manually
COPY --from=builder /usr/src/gtest/lib/*.a /usr/lib/

USER ubuntu
WORKDIR /home/ubuntu
