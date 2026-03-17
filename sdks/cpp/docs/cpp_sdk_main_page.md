# Catena C++ SDK

## Toolchain Recommendations

* [Ubuntu 22.04](linux_toolchain.md)
* [Mac OS](macos_toolchain.md)
* [Windows MSVS](windows_toolchain.md)

## Connect to DashBoard

DashBoard sends an HTTP GET request to /connect/connection-props.xml at port 80.
There is a ConnectionProps class inside common that listens on a default (80) but specifiable port.
It serves a specified body as a reply to requests to the /connect/connection-props.xml endpoint.
Both GRPC and REST one_of_everything currently uses the ConnectionProps class to serve connection info.

Once toolchain is configured, use cmake to build 'makefiles' and then compiler (make, xcode, msbuild) to build targets.
