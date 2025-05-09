# Dev Containers

## file break down

### docker containers

- a base toolchain
- - this has all the tools needed to build Catena
- a dev container
- - this has Catena and has been built once
- - this is setup for uses as a dev container
- a service container
- - this container is a minifide compiled version of Cantena

### toolchain

the toolchain is generated from a docker file installing tools defined in [toolchain-cpp.dockerfile](https://github.com/rossvideo/Catena/blob/develop/.devcontainer/toolchain-cpp.dockerfile)  
The version of each tool is defined in [toolchain-cpp.env](https://github.com/rossvideo/Catena/blob/develop/.devcontainer/toolchain-cpp.env)

### devcontainer.json
- this is the settings for vscode / curser to use when creating the dev container

## Setup
1. Install wsl

2. Install the Dev container extension on your host systems vscode / curser
```sh
code --install-extension ms-vscode-remote.remote-containers
```

3. Run Powershell script to generate WSL ubuntu instance and setup docker
```sh
.\wsl.ps1 $USERNAME $GITHUB_SIGNING_NAME $ROSS_EMAIL
```

4. Setup docker inside WSL

From inside Cursor,
```sh
CTRL + SHIFT + P -> Dev Containers: Rebuild without cache and Reopen in Container
```
Once that finishes, as instructed by the terminal, close cursor  
Run command from Powershell
```sh
wsl -d CatenaUbuntu
```
from inside the WSL shell
```sh
cd ~/Catena
code .
```
From inside Cursor,
```sh
CTRL + SHIFT + P -> Dev Containers: Rebuild without cache and Reopen in Container
```
5. Setup ninja builds

From inside the WSL shell in Cursor
```sh
cd  ~/Catena/sdk/cpp/build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCONNECTIONS='gRPC;REST' -DCMAKE_INSTALL_PREFIX=/usr/local/.local ~/Catena/sdks/cpp
ninja clean
ninja
```
6. your good to dev now :D
