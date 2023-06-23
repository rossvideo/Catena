# Linux Toolchain Installation {#Linux_toolchain}

This toolchain installation has been verified on x64 Ubuntu 22.04.2 LTS

## Getting a C++ compiler

`sudo apt-get install build-essential`

## Installing cmake

1. `sudo apt-get install cmake`

## Install and Build gRPC and Protobufs

Carefully follow [these steps](https://grpc.io/docs/languages/cpp/quickstart/)

## Install and Build jwt-cpp

1. Install openSSL eg. `sudo apt-get install libssl-dev`

2. Run `git clone https://github.com/Thalhammer/jwt-cpp && cd jwt-cpp && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ..  && make && sudo make install`

## Optionally Install doxygen

`sudo apt-get install doxygen`

### graphviz installation

Doxygen can add inheritance diagrams and call trees if the program called `dot`
which is part of graphviz is installed.

`sudo apt-get install graphviz`
