# Dev Containers


## Docker Containers

| Container Type        | Description                                                                     |
| --------------------- | ------------------------------------------------------------------------------- |
| **Base Toolchain**    | Has all tools needed to build Catena                                            |
| **Dev Container**     | Has Catena checked out and built once; used for day-to-day development.         |
| **Service Container** | Compiled Catena runtime image used as a service container.                      |

### File Overview

| File                     | Role / Purpose                                                                 |
| ------------------------ | ------------------------------------------------------------------------------ |
| `compose.yml`            | Defines the multi-container dev environment (base/toolchain, dev, services).  |
| `devcontainer.dockerfile` | Dockerfile for the main dev container                                        |
| `devcontainer.env`       | Environment variables used when building/running the dev container.           |
| `devcontainer.json`      | VS Code / Cursor dev-container configuration (features, settings, mounts).    |
| `toolchain.dockerfile`   | Dockerfile for the base toolchain image (installs build tools only).          |
| `toolchain.env`          | Versions and configuration for the toolchain tools used in `toolchain.dockerfile`. |

# --- TODO ---


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
cd ~/Catena/smpte
./build-openapi.sh
cd  ~/Catena/build/cpp
ninja
```
6. your good to dev now :D