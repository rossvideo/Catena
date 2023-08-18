# Windows Toolchain Installation (MSVC)

This toolchain installation has been verified on x64 Windows 10.

## Getting a C++ compiler

1. Install [Visual Studio build tools](https://visualstudio.microsoft.com/downloads/) for C++ dev
2. (Optional) Add msbuild to path. Usually under `%ProgramFiles%\Microsoft Visual Studio\<YEAR>\<YOUR_VS_EDITION>\MSBuild\Current\Bin`

## Installing cmake

* Install [cmake](https://cmake.org/download/)

## Install and Build jwt-cpp

1. Install full openSSL from [here](https://slproweb.com/products/Win32OpenSSL.html)
2. Run `git clone https://github.com/Thalhammer/jwt-cpp && cd jwt-cpp && mkdir build && cd build`
3. Run `cmake -DCMAKE_BUILD_TYPE=Release ..` and then `msbuild jwt-cpp.sln`
4. Find location of `jwt-cpp-targets.cmake` and create symlink to `build/jwt-cpp-targets.cmake` eg. `mklink jwt-cpp-targets.cmake c:\...\build\CMakeFiles\Export\272ceadb8458515b2ae4b5630a6029cc\jwt-cpp-targets.cmake`

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

## Optionally Install doxygen

* Install [doxygen](https://www.doxygen.nl/download.html)

### graphviz installation

Doxygen can add inheritance diagrams and call trees if the program called `dot`
which is part of graphviz is installed.

* Install [graphviz](https://graphviz.org/download/) (add to path during installation)

## Building SDK Example

```
cd sdks/cpp
md build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<vcpkg_install>/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=<jwt-cpp-install>/build
msbuild CATENA_CPP_SDK.sln
```


