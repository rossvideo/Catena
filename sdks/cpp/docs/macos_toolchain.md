# macos Toolchain Installation

This toolchain installation has been verified on macOS Sequoia 15.0.x
on a M1 Pro equipped macbook pro.

## Getting a C++ compiler

`xcode-select --install`

verify with...

```bash
c++ --version
Apple clang version 16.0.0 (clang-1600.0.26.6)
Target: arm64-apple-darwin24.3.0
Thread model: posix
InstalledDir: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin
```

## Installing cmake

1. install the GUI app [from here](https://cmake.org/download/)
2. update your path to provide command line access to cmake

In `~/.zprofile` ...

```bash
PATH="/Applications/CMake.app/Contents/bin":"$PATH"
source ~/.zprofile

# verify with ...
cmake --version
cmake version 3.28.0

CMake suite maintained and supported by Kitware (kitware.com/cmake).
```

## Dependencies

Instead of using the system versions of libraries used such as OpenSSL, we get better control over their versioning by installing them under $HOME/.local and directing dependent libraries, such as gRPC to use the $HOME/.local version instead of the system version.

### Clone, build and install OpenSSL

```bash
curl -O https://www.openssl.org/source/openssl-3.2.1.tar.gz
tar xzf openssl-3.2.1.tar.gz
cd openssl-3.2.1

# set up the local install target and OS version
./Configure --prefix=$HOME/.local darwin64-arm64-cc -mmacosx-version-min=15.0
make
make install
```

### Install and Build gRPC and Protobufs

Carefully follow [these steps](https://grpc.io/docs/languages/cpp/quickstart/) but use the instructions below for the cmake arguments.

```bash
 cmake ../.. -DgRPC_INSTALL=ON    \
 -DCMAKE_BUILD_TYPE=Release       \
 -DgRPC_ABSL_PROVIDER=package     \
 -DgRPC_CARES_PROVIDER=module     \
 -DgRPC_PROTOBUF_PROVIDER=package \
 -DgRPC_RE2_PROVIDER=module       \
 -DgRPC_SSL_PROVIDER=package      \
 -DgRPC_ZLIB_PROVIDER=module      \
 -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk \
 -DCMAKE_PREFIX_PATH=/Users/$USER/.local \
 -DOPENSSL_ROOT_DIR=/Users/$USER/.local \
 -DOPENSSL_CRYPTO_LIBRARY=/Users/$USER/.local/lib/libcrypto.a \
 -DOPENSSL_SSL_LIBRARY=/Users/$USER/.local/lib/libssl.a \
 -DCMAKE_INSTALL_PREFIX=/Users/$USER/.local

 make -j $(sysctl -n hw.ncpu)
 make install
```



### Install homebrew

Several of the other dependencies are installed using homebrew.

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```


### Clone, build and install jwt-cpp into the $HOME/.local folder

```bash
cd $HOME
git clone https://github.com/Thalhammer/jwt-cpp && cd jwt-cpp && mkdir build && cd build
 cmake .. \
 -DCMAKE_INSTALL_PREFIX=$HOME/.local \
 -DCMAKE_BUILD_TYPE=Release \
 -DJWT_BUILD_EXAMPLES=OFF \
 -DJWT_BUILD_TESTS=OFF \
 -DJWT_DISABLE_PICOJSON=OFF \
 -DJWT_EXTERNAL_NLOHMANN_JSON=ON
 
```

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