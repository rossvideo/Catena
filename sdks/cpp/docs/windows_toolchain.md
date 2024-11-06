# Windows Toolchain Installation (MSVC)

This toolchain installation has been verified on x64 Windows 10.

## Getting a C++ compiler

1. Install [Visual Studio build tools](https://visualstudio.microsoft.com/downloads/) for C++ dev
2. (Optional) Add msbuild to path. Usually under `%ProgramFiles%\Microsoft Visual Studio\<YEAR>\<YOUR_VS_EDITION>\MSBuild\Current\Bin`

## Installing cmake

* Install [cmake](https://cmake.org/download/)

## Dependencies

### Install and Build gRPC and Protobufs

1. Install [vcpkg](https://vcpkg.io/en/getting-started)
2. Navigate to install directory of vcpkg and run
```
vcpkg install grpc:x64-windows
vcpkg install protobuf protobuf:x64-windows
vcpkg install protobuf[zlib] protobuf[zlib]:x64-windows
vcpkg integrate install
```

### Install and Build jwt-cpp

1. Install full openSSL from [here](https://slproweb.com/products/Win32OpenSSL.html)
2. Run `git clone https://github.com/Thalhammer/jwt-cpp && cd jwt-cpp && mkdir build && cd build`
3. Run `cmake -DCMAKE_BUILD_TYPE=Release ..` and then `msbuild jwt-cpp.sln`
4. Find location of `jwt-cpp-targets.cmake` and create symlink to `build/jwt-cpp-targets.cmake` eg. `mklink jwt-cpp-targets.cmake c:\...\build\CMakeFiles\Export\272ceadb8458515b2ae4b5630a6029cc\jwt-cpp-targets.cmake`

> If the build fails with syntax errors follow [this](https://github.com/Thalhammer/jwt-cpp/blob/master/docs/faqs.md#building-on-windows-fails-with-syntax-errors)
___
> After installing the dependencies above you will have to add `-DCMAKE_TOOLCHAIN_FILE=<vcpkg_install>/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=<jwt-cpp-install>/build` to cmake options in your build environment

### Install boost if using the REST connection manager

```
.\vcpkg.exe install boost
```

## Optionally Install doxygen

* Install [doxygen](https://www.doxygen.nl/download.html)

### graphviz installation

Doxygen can add inheritance diagrams and call trees if the program called `dot`
which is part of graphviz is installed.

* Install [graphviz](https://graphviz.org/download/) (add to path during installation)

## Optionally Install Google Test

1. Download the library ZIP file from the [Github repo](https://github.com/google/googletest/tree/release-1.10.0).  Extract the ZIP file to a directory on your computer.
2. Open command prompt and navigate to the directory where you extracted Google Test and run

```
mkdir build
cd build
cmake ..
```

To build without unit test set UNIT_TESTING=OFF when building
