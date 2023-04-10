# macos Toolchain Installation {#macos_toolchain}

This toolchain installation has been verified on macOS Ventura 13.2.1
on a M1 Pro equipped macbook pro.

## Getting a C++ compiler

`xcode-select --install`

verify with...

```{.sh}
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

```{.sh}
PATH="/Applications/CMake.app/Contents/bin":"$PATH"
```

```{.sh}
source ~/.zprofile
```

## Install and Build gRPC and Protobufs

Carefully follow [these steps](https://grpc.io/docs/languages/cpp/quickstart/)

## Optionally install doxygen

Note that this app isn't signed so, by default it cannot be installed.

You can work around this, at your own risk, by `ctrl+click` on the `.dmg` downloaded [from here](https://www.doxygen.nl/files/Doxygen-1.9.6.dmg)

Add it to your path, in `~/.zprofile` ...

```{.sh}
"/Applications/Doxygen.app/Contents/Resources/":"$PATH"
```

```{.sh}
source ~/.zprofile
```

### graphviz installation

Doxygen can add inheritance diagrams and call trees if the program called `dot`
which is part of graphviz is installed.

It needs to be installed using [brew](https://docs.brew.sh/Installation),
which itself may need installation if not already present

```{.sh}
brew install graphviz
```
