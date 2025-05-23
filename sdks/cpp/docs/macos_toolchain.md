# macos Toolchain Installation

This toolchain installation has been verified on macOS Ventura 13.2.1, Sonoma 14.x and Sequoia 15.0.1
on a M1 Pro equipped macbook pro.

## Getting a C++ compiler

`xcode-select --install`

verify with...

```bash
c++ --version
Apple clang version 14.0.0 (clang-1400.0.29.202)
Target: arm64-apple-darwin22.3.0
Thread model: posix
InstalledDir: /Library/Developer/CommandLineTools/usr/bin
```

## Installing cmake

1. install the GUI app [from here](https://cmake.org/download/)
2. update your path to provide command line access to cmake

In `~/.zprofile` ...

```bash
PATH="/Applications/CMake.app/Contents/bin":"$PATH"
```

```bash
source ~/.zprofile
```

## Dependencies

### Install and Build gRPC and Protobufs

Carefully follow [these steps](https://grpc.io/docs/languages/cpp/quickstart/)

or [these](https://github.com/grpc/grpc/blob/master/BUILDING.md) which are a bit different.

After upgrading to MacOS 13.3, these instructions no longer worked. I had to run this command to create the build folder.

```bash
% cmake ../.. -DgRPC_INSTALL=ON                \
              -DCMAKE_BUILD_TYPE=Release       \
              -DgRPC_ABSL_PROVIDER=package     \
              -DgRPC_CARES_PROVIDER=package    \
              -DgRPC_PROTOBUF_PROVIDER=package \
              -DgRPC_RE2_PROVIDER=package      \
              -DgRPC_SSL_PROVIDER=package      \
              -DgRPC_ZLIB_PROVIDER=package \
              -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk \
              -DCMAKE_PREFIX_PATH=/Users/$USER/.local 
```

### Install homebrew

Several of the other dependencies are installed using homebrew.

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```


### Install and Build jwt-cpp

1. Install openSSL eg. `brew install openssl`
2. Run `git clone https://github.com/Thalhammer/jwt-cpp && cd jwt-cpp && mkdir build && cd build`
3. Run `cmake -DCMAKE_BUILD_TYPE=Release ..`
4. Build and add to path

## Optionally install doxygen

Note that this app isn't signed so, by default it cannot be installed.

You can work around this, at your own risk, by `ctrl+click` on the `.dmg` downloaded [from here](https://www.doxygen.nl/files/Doxygen-1.9.6.dmg)

Add it to your path, in `~/.zprofile` ...

```bash
"/Applications/Doxygen.app/Contents/Resources/":"$PATH"
```

```bash
source ~/.zprofile
```

### graphviz installation

Doxygen can add inheritance diagrams and call trees if the program called `dot`
which is part of graphviz is installed.

It needs to be installed using [brew](https://docs.brew.sh/Installation),
which itself may need installation if not already present

```bash
brew install graphviz
```
## Optionally install Google Test

It can be installed using [brew](https://docs.brew.sh/Installation),
which itself may need installation if not already present

`brew install googletest`

To build without Google Test, empty the build folder and run
`cmake .. -DUNIT_TESTING=OFF`

## If you're targeting REST, install boost, and asio

```bash
brew install boost && brew install asio
```