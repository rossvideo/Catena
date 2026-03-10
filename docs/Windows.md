::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# Catena Windows Build

## Required Tools

Before working with Catena in Windows, you must first ensure that all of the necessary tools are installed,

- [ ] **LLVM/Clang** — Install from <https://github.com/llvm/llvm-project/releases> (clang, clang++, lld). Add to `PATH`.
- [ ] **Visual Studio Build Tools** — Required for Windows SDK headers/libs even when using Clang. Install "Desktop development with C++" workload from <https://visualstudio.microsoft.com/downloads/>.
- [ ] **CMake ≥ 3.20** — Install from <https://cmake.org/download/>. Add to `PATH`.
- [ ] **Ninja** — Install from <https://github.com/ninja-build/ninja/releases>. Add to `PATH`.
- [ ] **Node.js ≥ 22.x** — Install from <https://nodejs.org/>. Required for codegen tools.
- [ ] **Git** — Install from <https://git-scm.com/> (includes Git Bash, needed for shell scripts in the build).
- [ ] **OpenSSL** — Install full OpenSSL from <https://slproweb.com/products/Win32OpenSSL.html>. Required by jwt-cpp and gRPC.
- [ ] **vcpkg** — Clone <https://github.com/microsoft/vcpkg> and bootstrap: `.\bootstrap-vcpkg.bat`. Further instructions below.
- [ ] **pkg-config** (optional) — `vcpkg install pkgconf:x64-windows` or install via chocolatey. Only needed if cmake `PkgConfig` module is invoked (now conditionally skipped on Windows). Included in below command, can remove if you don't want it.
- [ ] **jwt-cpp** — Clone <https://github.com/Thalhammer/jwt-cpp>. Further instructions below.
- [ ] **yq** — `winget install --id MikeFarah.yq` or `choco install yq`

### vcpkg installation
 
With the above tools installed, you now need to clone vcpkg and use it to install some more packages. These installations may take a while. It is also important to note that if building Catena in debug, One_of_everthing examples will only work if installing the static versions instead.

```powershell
git clone https://github.com/microsoft/vcpkg C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install boost-system:x64-windows boost-date-time:x64-windows boost-url:x64-windows boost-asio:x64-windows boost-program-options:x64-windows curl:x64-windows zlib:x64-windows asio:x64-windows gtest:x64-windows abseil:x64-windows grpc:x64-windows protobuf:x64-windows protobuf[zlib]:x64-windows pkgconf:x64-windows --vcpkg-root .
# If building in debug and running one_of_everything
.\vcpkg install boost-system:x64-windows-static boost-date-time:x64-windows-static boost-url:x64-windows-static boost-asio:x64-windows-static boost-program-options:x64-windows-static curl:x64-windows-static zlib:x64-windows-static asio:x64-windows-static gtest:x64-windows-static abseil:x64-windows-static grpc:x64-windows-static protobuf:x64-windows-static protobuf[zlib]:x64-windows-static pkgconf:x64-windows-static --vcpkg-root .
```

### jwt-cpp installation

Build both Debug and Release installs so Catena can use the matching one for each build type. From a Visual Studio developer prompt:

```powershell
cd "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build" 
.\vcvars64.bat
git clone https://github.com/Thalhammer/jwt-cpp C:\jwt-cpp
cd C:\jwt-cpp
git checkout v0.7.1
mkdir build-clangcl-all; cd build-clangcl-all
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="C:/PROGRA~1/LLVM/bin/clang-cl.exe" -DCMAKE_CXX_COMPILER="C:/PROGRA~1/LLVM/bin/clang-cl.exe" -DCMAKE_POLICY_VERSION_MINIMUM="3.5" ..
ninja
cmake --install . --prefix C:/jwt-cpp/install-clangcl-all
```

## Building Catena

### Setting up Catena repo
You need to create and setup a local repository, if you've already done this then skip ahead to the next section.
```powershell
git clone https://github.com/rossvideo/Catena.git C:\<Catena-root>
cd C:\<Catena-root>
git checkout windows-build
git submodule update --init
cd tools\codegen
npm install --yes
```

### Building the project
If using a pre-existing repo, you may run into cache issues which can be fixed by clearing build\cpp using `rmdir cpp`. If you installed static packages, set `VCPKG_TARGET_TRIPLET` to `x64-windows-static` instead.

```powershell
# from C:\<Catena-root>
mkdir build; cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCONNECTIONS="gRPC;REST" -DCOVERAGE=OFF -DCMAKE_INSTALL_PREFIX=install -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -DLOG_DIR="../logs" -B cpp -DCATENA_UNITTESTS_DIR="../unittests" -S ../sdks/cpp -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_PREFIX_PATH="C:/jwt-cpp/install-clangcl-all" -Djwt-cpp_DIR="C:/jwt-cpp/install-clangcl-all/cmake" -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_C_COMPILER="C:/PROGRA~1/LLVM/bin/clang-cl.exe" -DCMAKE_CXX_COMPILER="C:/PROGRA~1/LLVM/bin/clang-cl.exe" -DCMAKE_RC_COMPILER="C:/PROGRA~1/LLVM/bin/llvm-rc.exe" -DCMAKE_MT="C:/PROGRA~1/LLVM/bin/llvm-mt.exe"
cmake --build cpp
```
