# Running VSCode Workspace With Docker

## Extensions Required
[Docker](https://marketplace.visualstudio.com/items?itemName=ms-azuretools.vscode-docker) and follow the Docker install instructions listed if it is not already set up.
[Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) for integration with the Catena workspace.
[Cmake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)

## Steps to run
Right click on the [Dockerfile](./Dockerfile) within skds/cpp/docker and select the option build.

On completion, press F1 to open the command palette and select the option 
` Dev Containers: Open worspace in Container `

Ensure cmake tools is enabled in the container as well as any ones you might prefer to use 

Continue to sdks/cpp/build, making sure it is empty and then using 
` cmake -G Ninja -DUNIT_TESTING=OFF .. && ninja ` 
using -DUNIT_TESTING=ON to build with Google Test enabled