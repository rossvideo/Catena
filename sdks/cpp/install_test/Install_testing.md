# Testing Catena Install
The install_test directory is a mini project to test that the installing of the Catena package is working properly. This example uses an exact copy of the gRPC status update example with the only difference being that it is built using the installed Catena package.

Here are the steps to build this project
```
cd install_test
mkdir build && mkdir test_install_dir
cd ../build
cmake .. -G Ninja \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCONNECTIONS="gRPC;" \
    -DCMAKE_INSTALL_PREFIX=/home/johndanen/Workspace/Catena/sdks/cpp/install_test/test_install_dir
ninja install
cd ../install_test/build
cmake .. -G Ninja
ninja
```