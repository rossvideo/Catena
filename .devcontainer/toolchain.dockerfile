FROM library/ubuntu:24.04 AS builder
LABEL org.opencontainers.image.title="Catena Toolchain"
LABEL org.opencontainers.image.description="The toolchain for building Catena"
LABEL org.opencontainers.image.version="v0.0.1-dev"
LABEL org.opencontainers.image.vendor="Rossvideo"

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Copy the toolchain.env file
COPY toolchain.env /root/toolchain.env

# Source the toolchain.env file
RUN . /root/toolchain.env \
    && apt-get update && apt-get install -y \
    build-essential sudo cmake=$CMAKE_VERSION nodejs=$NODEJS_VERSION npm git=$GIT_VERSION libssl-dev=$OPENSSL_VERSION doxygen=$DOXYGEN_VERSION graphviz=$GRAPHVIZ_VERSION \
    libgtest-dev=$GTEST_VERSION libboost-all-dev=$BOOST_VERSION libasio-dev=$ASIO_VERSION libgmock-dev=$GMOCK_VERSION ninja-build=$NINJA_VERSION gdb=$GNU_DEBUGGER_VERSION \
    && npm install -g n \
    && apt-get clean

# Install gRPC
RUN . ~/toolchain.env \
    && git clone -b $GRPC_VERSION https://github.com/grpc/grpc ~/grpc \
    && cd ~/grpc && git submodule update --recursive --init \
    && mkdir -p ~/grpc/cmake/build \
    && cd ~/grpc/cmake/build \
    && cmake ~/grpc -DCMAKE_BUILD_TYPE=Release -DgRPC_INSTALL=ON -DCMAKE_INSTALL_PREFIX=~/.local \
    && make -j4 install

# Install jwt-cpp
RUN . ~/toolchain.env \
    && git clone -b $JWT_CPP_VERSION https://github.com/Thalhammer/jwt-cpp ~/jwt-cpp \
    && mkdir ~/jwt-cpp/build \
    && cd ~/jwt-cpp/build \
    && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/.local ~/jwt-cpp \
    && make && make install

# Install Google Test
RUN cd /usr/src/gtest \
    && sudo cmake -DCMAKE_INSTALL_PREFIX=~/.local CMakeLists.txt \
    && sudo make && sudo cp lib/*.a /usr/lib

