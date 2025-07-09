FROM library/ubuntu:24.04 AS builder
LABEL org.opencontainers.image.title=$IMAGE_TITLE
LABEL org.opencontainers.image.description=$IMAGE_DESCRIPTION
LABEL org.opencontainers.image.version=$IMAGE_VERSION
LABEL org.opencontainers.image.vendor=$IMAGE_VENDOR

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Copy the toolchain.env file
COPY toolchain-cpp.env /root/toolchain.env

# Source the toolchain.env file
RUN . /root/toolchain.env \
    && apt-get update && apt-get install -y \
    build-essential sudo \
    cmake=$CMAKE_VERSION \
    nodejs=$NODEJS_VERSION \
    npm git \
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
    && npm install -g n \
    && apt-get install -y libgoogle-glog-dev \
    && apt-get clean

# Install gRPC
RUN . /root/toolchain.env \
    && git clone -b $GRPC_VERSION https://github.com/grpc/grpc /usr/local/grpc \
    && cd /usr/local/grpc && git submodule update --recursive --init \
    && mkdir -p /usr/local/grpc/cmake/build \
    && cd /usr/local/grpc/cmake/build \
    && cmake /usr/local/grpc -DCMAKE_BUILD_TYPE=Release -DgRPC_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/usr/local/.local \
    && make -j4 install

# Install jwt-cpp
RUN . /root/toolchain.env \
    && git clone -b $JWT_CPP_VERSION https://github.com/Thalhammer/jwt-cpp /usr/local/jwt-cpp \
    && mkdir /usr/local/jwt-cpp/build \
    && cd /usr/local/jwt-cpp/build \
    && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local/.local /usr/local/jwt-cpp \
    && make && make install

# Install Google Test
RUN cd /usr/src/gtest \
    && sudo cmake -DCMAKE_INSTALL_PREFIX=/usr/local/.local CMakeLists.txt \
    && sudo make && sudo cp lib/*.a /usr/lib

# Install Docker & Docker Compose
RUN . /root/toolchain.env \
    && sudo apt-get update \
    && sudo apt-get install ca-certificates curl \
    && sudo install -m 0755 -d /etc/apt/keyrings \
    && sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc \
    && sudo chmod a+r /etc/apt/keyrings/docker.asc \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}") stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null \
    && sudo apt-get update \
    && sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

# Install Python and gcovr for coverage reports
RUN sudo apt-get update && sudo apt-get install -y python3-pip \
    && sudo pip3 install --no-cache-dir gcovr --break-system-packages