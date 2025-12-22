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

# Setup
1. Open PowerShell in Administrator mode and install [WSL](https://learn.microsoft.com/en-us/windows/wsl/install)
```sh
wsl --install
```

2. Install the Dev Container extension on your host systems IDE (VScode / Cursor)
```sh
code --install-extension ms-vscode-remote.remote-containers
```

3. In the Catena codebase Navigate to the `.devcontainer` directory
```sh
cd ~/Catena/.devcontainer
```

4. Run the PowerShell script to generate a WSL Ubuntu instance and setup Docker
```sh
.\wsl.ps1 $USERNAME $GITHUB_SIGNING_NAME $ROSS_EMAIL
```

5. Setup SMPTE
```sh
cd ~/Catena/smpte
./build-openapi.sh
```


6. Build Container
```sh
CTRL + SHIFT + P -> Dev Containers: Rebuild without cache and Reopen in Container
```

7. Select GO or C++ Dev Container

![Container Options](/docs/images/Container_Options.png)

8. Build

| Language              | Build Command                        |
| --------------------- | -------------------------------------|
| **C++**               | ```cd ~/Catena/build/cpp && ninja``` |
| **GO**                | ```cd ~/Catena/sdks/go && make```    |


9. You're good to dev now :D
