# Catena example servers

## hello_world

### How to build

Setup docker environment following steps outlined [here](https://github.com/rossvideo/Catena/blob/develop/.devcontainer/README.md).
From inside the WSL shell in docker container
```sh
cd  ~/Catena/sdk/cpp/build
ninja clean
ninja
```

### How to run

From inside the WSL shell in docker container
```sh
cd  ~/Catena/sdk/cpp/build
common/examples/hello_world/hello_world
```

### What it does

1. Device is setup  
2. 'hello' parameter is read and printed out  
3. Parameter value is changed to "Goodbye, Cruel World!"  
4. Parameter is printed out and the new value appears  
5. Repeat with severral different parameters of different types
6. Disconnect device