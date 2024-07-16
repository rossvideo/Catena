# Cross Compiling
This doc will cover the basics of cross-compiling compiling the dependencies for Catena. This doc is written for Linux but the process should be similar for other systems

First you will need a cmake toolchain file that describes the target system. Creating the cmake toolchain will not be covered here. The cmake toolchain should define a variable called CMAKE_FIND_ROOT_PATH. Make sure to make note of what this path is set to because this will be the directory where all cross compiled dependencies will need to be installed.

For these examples we will assume this path was set to "/path/to/target_root"

## Cross Compile OpenSSL 
```
git clone https://github.com/openssl/openssl.git
cd openssl
./Configure linux-x86_64 \ 
	--cross-compile-prefix=path/to/toolchainfile.cmake \
	--prefix=/path/to/target_root \
	-static
make install
```

## Cross Compile Abseil
There is currently an issue where gRPC does not include the correct version Abseil in its third party libraries. To work around this problem, install Abseil manually, then build gRPC with '-DgRPC_ABSL_PROVIDER=package'

For more info see the issue on [GitHub](https://github.com/grpc/grpc/issues/35854)

```
git clone -b lts_2024_01_16 https://github.com/abseil/abseil-cpp.git
cd abseil-cpp/CMake
mkdir cross_build
cd cross_build
cmake ../.. \
	-DCMAKE_BUILD_TYPE=Release   \
	-DABSL_ENABLE_INSTALL=ON \
	-DCMAKE_TOOLCHAIN_FILE=/path/to/toolchainfile.cmake \
    -DCMAKE_INSTALL_PREFIX=/path/to/target_root
make install
```

## Cross Compiling gRPC
NOTE: cross compiling gRPC requires the host system to already have gRPC installed. This is because the the gRPC build process needs to be able to run the grpc_cpp_plugin and the protoc compiler to generate C++ source files to be cross compiled.

If not already done, install gRPC on the host system like this:
```
git clone -b v1.64.2 https://github.com/grpc/grpc
cd grpc
git submodule update --recursive --init
mkdir -p cmake/build
cd cmake/build
cmake ../.. 
make install
```

Now that gRPC is installed on the host system, We can cross compile it for the target system
```
cd ~/grpc
mkdir -p cmake/cross_build
cd cmake/cross_build
cmake ../.. \
	-D CMAKE_BUILD_TYPE=Release   \
	-DgRPC_INSTALL=ON  \
	-DgRPC_ABSL_PROVIDER=package \
	-DgRPC_SSL_PROVIDER=package \
	-D CMAKE_TOOLCHAIN_FILE=/path/to/toolchainfile.cmake  \
	-D CMAKE_INSTALL_PREFIX=/path/to/target_root \
    -D_gRPC_CPP_PLUGIN=$ENV{home}/.local/bin/grpc_cpp_plugin \
    -D_gRPC_PROTOBUF_PROTOC=$ENV{home}/.local/bin/protoc
make install
```

## Cross Compiling JWT-CPP
```
git clone https://github.com/Thalhammer/jwt-cpp && cd jwt-cpp
mkdir cross_build && cd cross_build
cmake .. \
    -D CMAKE_BUILD_TYPE=Release   \
    -D CMAKE_TOOLCHAIN_FILE=/path/to/toolchainfile.cmake  \
    -D CMAKE_INSTALL_PREFIX=/path/to/target_root
make install
```

## Cross Compiling Catena
```
cd ~/Catena/sdks/cpp
mkdir build && cd build
cmake .. -G Ninja \
	-D CMAKE_BUILD_TYPE=Release   \
	-D CONNECTIONS=gRPC \
    -D CMAKE_TOOLCHAIN_FILE=/path/to/toolchainfile.cmake
ninja
```
