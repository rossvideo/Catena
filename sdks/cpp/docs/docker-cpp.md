# Using Docker With The Catena C++ SDK

## Terminal
### Requirements
- Install [Docker](https://docs.docker.com/get-docker/) and add it to your system path

- If on Linux, make sure to [enable docker as non-root user](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user) 

In the Catena/sdks/cpp/docker/ directory run `./dev-start.sh <absolute/path/to/Catena>`

***Note***: if you get the error:
``` exit status 22: unpigz: abourt: zlib version less than 1.2.3``` download the [latest version of pigz](https://zlib.net/pigz/).


## VSCode Workspaces
### Extensions Required
- [Docker](https://marketplace.visualstudio.com/items?itemName=ms-azuretools.vscode-docker) additionally follow the Docker software install instructions listed in the extension if not already set up.
- [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) for integration with the Catena workspace.
- [Cmake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) for build setup

### Steps to run
<!-- create a environment variable `DOCKER_GID` and set it to your system's docker group id.  -->
<!-- On Linux you can get this using `cut -d: -f3 < <(getent group docker)`. -->

Press F1 to open the command palette and select the option 
` Dev Containers: Open worspace in Container `, navigating to your Catena workspace file

***Note***: if you get the error:
``` exit status 22: unpigz: abourt: zlib version less than 1.2.3``` download the [latest version of pigz](https://zlib.net/pigz/).

Ensure the C/C++ Extension pack is enabled in the container as well as any other extensions you might want.

Continue to sdks/cpp/build, emptying it and using 
``` cmake -G Ninja -DCONNECTIONS=gRPC -DCATENA_MODELS=lite .. ```

<!-- ***Note***: set `-DUNIT_TESTING=ON` to build with Google Test enabled -->