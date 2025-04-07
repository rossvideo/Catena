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

### toolchain.env
- this conatined the versions of all the tools beening used in the toolchain conatiner and the default branch of Catena for the dev conatiner

### devcontainer.json
- this is the settings for vscode / curser to use when creating the dev container

## Setup
1. be on not windows (in a wsl is ok)

2. install the Dev conatiner extenshion on your host systems vscode / curser
```sh
code --install-extension ms-vscode-remote.remote-containers
```

3. install docker and docker compose
```sh
# for ubuntu 24.04
## setup the certs for docker
sudo apt-get update
sudo apt-get install ca-certificates curl
sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc

# Add the repository to Apt sources:
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
  $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}") stable" | \
  sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt-get update
```
```sh
sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

sudo groupadd docker
sudo usermod -aG docker $USER
newgrp docker
sudo systemctl enable docker.service
sudo systemctl enable containerd.service
sudo service docker start
```

4. pull the repo
5. ctl+shift+p `Dev Containers: Rebuild Without Cache and Reopen in Container`
6. your good to dev now :D
