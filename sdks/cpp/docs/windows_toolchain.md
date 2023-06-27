# Windows Toolchain Installation {#windows_toolchain}

# TODO: This is still a work in progress
This toolchain installation has been verified on Windows 10

## Getting a C++ compiler

Download and install CMake release from: https://cmake.org/download/

Download and install NASM from: https://www.nasm.us/
Add $HOME\AppData\Local\bin\NASM to PATH

Download and install MinGW compiler installer from: https://osdn.net/projects/mingw/
Add the mingw32-automakeX.XX-bin and mingw32-autoconf-bin packages
Add MinGw bin directory to PATH variable.


## Building and installing grpc
Create the following directories in the HOME directory: ...\.local\bin
Add the \bin directory to PATH

In GitBash execute:
`git clone --recurse-submodules -b v1.55.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc`
`cd grpc`
`mkdir -p cmake/build`
`pushd cmake/build`
`cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX="$HOME/.local/ -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="/c/MinGw/bin/g++.exe" -DCMAKE_C_COMPILER="/c/MinGw/bin/gcc.exe" ../..`
`mingw32-make.exe -j 4`
`make install`
`popd`

## Building the C++ SDK
Go to sdks/cpp directory
Run `cmake -G "MinGW Makefiles" CMakeLists.txt`