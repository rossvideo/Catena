# Windows Toolchain Installation

This toolchain installation has been verified on x64 Windows 10

## Getting a C++ compiler

* Install [Visual Studio build tools](https://visualstudio.microsoft.com/downloads/)

## Installing cmake

* Install [cmake](https://cmake.org/download/)

## Install and Build jwt-cpp

1. Download and install full openSSL from [here](https://slproweb.com/products/Win32OpenSSL.html)
1. Open a developer command prompt for vs
2. Run `git clone https://github.com/Thalhammer/jwt-cpp && cd jwt-cpp && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ..  && msbuild jwt-cpp.sln`
3. Find location of `jwt-cpp-targets.cmake` and create symlink to `build/jwt-cpp-targets.cmake` eg. `mklink jwt-cpp-targets.cmake c:\...\build\CMakeFiles\Export\272ceadb8458515b2ae4b5630a6029cc`

> If the build fails with syntax errors follow [this](https://github.com/Thalhammer/jwt-cpp/blob/master/docs/faqs.md#building-on-windows-fails-with-syntax-errors)

## Install and Build gRPC and Protobufs

1. Install [vcpkg](https://vcpkg.io/en/getting-started)
2. Navigate to install directory of vcpkg and run
```
vcpkg install grpc:x64-windows
vcpkg install protobuf protobuf:x64-windows
vcpkg install protobuf[zlib] protobuf[zlib]:x64-windows
vcpkg integrate install
```
3. Set `VCPKG_INSTALL` to the installation of vcpkg in env variables (no quotes or leading slash)
4. Navigate to sdks/cpp and run `md build && cd build`
5. Run `cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_INSTALL%/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=<jwt-cpp-install>/build` where `<jwt-cpp-install>` is path of jwt-cpp build folder
6. Open a developer command prompt for vs and navigate to build directory
7. Run `msbuild CATENA_CPP_SDK.sln` twice (proto files get built on first run)

## Optionally Install doxygen

**todo**

### graphviz installation

Doxygen can add inheritance diagrams and call trees if the program called `dot`
which is part of graphviz is installed.

**todo**
