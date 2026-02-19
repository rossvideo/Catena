# Catena C++ SDK — Windows Build TODO

Target: Windows x64, Clang (LLVM clang/clang++), both static (.lib) and dynamic (.dll) libraries, gRPC + REST connections.

---

## Host Tools to Install

- [ ] **LLVM/Clang** — Install from <https://github.com/llvm/llvm-project/releases> (clang, clang++, lld). Add to `PATH`.
- [ ] **Visual Studio Build Tools** — Required for Windows SDK headers/libs even when using Clang. Install "Desktop development with C++" workload from <https://visualstudio.microsoft.com/downloads/>.
- [ ] **CMake ≥ 3.20** — Install from <https://cmake.org/download/>. Add to `PATH`.
- [ ] **Ninja** — Install from <https://github.com/ninja-build/ninja/releases>. Add to `PATH`.
- [ ] **Node.js ≥ 22.x** — Install from <https://nodejs.org/>. Required for codegen tools.
- [ ] **Git** — Install from <https://git-scm.com/> (includes Git Bash, needed for shell scripts in the build).
- [ ] **vcpkg** — Clone <https://github.com/microsoft/vcpkg> and bootstrap: `.\bootstrap-vcpkg.bat`. Set `VCPKG_ROOT` env var.
- [ ] **OpenSSL** — Install full OpenSSL from <https://slproweb.com/products/Win32OpenSSL.html>. Required by jwt-cpp and gRPC.
- [ ] **pkg-config** (optional) — `vcpkg install pkgconf:x64-windows` or install via chocolatey. Only needed if cmake `PkgConfig` module is invoked (now conditionally skipped on Windows).
## Installing vcpkg

```powershell
git clone https://github.com/microsoft/vcpkg C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
```

## vcpkg Packages to Install
Run from your vcpkg directory:

```powershell
.\vcpkg install grpc:x64-windows
.\vcpkg install protobuf:x64-windows protobuf[zlib]:x64-windows
.\vcpkg install boost-system:x64-windows boost-date-time:x64-windows boost-url:x64-windows
.\vcpkg install curl:x64-windows
.\vcpkg install zlib:x64-windows
.\vcpkg install asio:x64-windows
.\vcpkg install gtest:x64-windows
.\vcpkg install abseil:x64-windows
```

## Manual Builds

- [ ] **jwt-cpp v0.7.1** — Build from source with OpenSSL:
  ```powershell
  "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
  git clone https://github.com/Thalhammer/jwt-cpp C:\jwt-cpp
  cd C:\jwt-cpp
  git checkout v0.7.1
  mkdir build-clangcl-all; cd build-clangcl-all
  cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="C:/PROGRA~1/LLVM/bin/clang-cl.exe" -DCMAKE_CXX_COMPILER="C:/PROGRA~1/LLVM/bin/clang-cl.exe" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..
  ninja
  ```
  Note the build dir for `CMAKE_PREFIX_PATH`.

## CMake Invocation (Windows)

```powershell
mkdir build; cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCONNECTIONS="gRPC;REST" -DCOVERAGE=OFF -DCMAKE_INSTALL_PREFIX=install -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -DLOG_DIR=../logs -B cpp -S ../sdks/cpp -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_PREFIX_PATH="C:/jwt-cpp/install-clangcl-all" -Djwt-cpp_DIR="C:/jwt-cpp/install-clangcl-all/cmake" -DCMAKE_C_COMPILER="C:/PROGRA~1/LLVM/bin/clang-cl.exe" -DCMAKE_CXX_COMPILER="C:/PROGRA~1/LLVM/bin/clang-cl.exe" -DCMAKE_RC_COMPILER="C:/PROGRA~1/LLVM/bin/llvm-rc.exe" -DCMAKE_MT="C:/PROGRA~1/LLVM/bin/llvm-mt.exe"


cmake --build cpp
```

## Steps Taken to Make Build Run (Executed)

1. Opened a VS Build Tools initialized environment (required so Windows SDK/RC/LIB are discoverable):

```cmd
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
```

2. Built and installed jwt-cpp for clang-cl:

```cmd
cd /d C:\jwt-cpp\build-clangcl-all
cmake --install . --prefix C:/jwt-cpp/install-clangcl-all
```

3. Installed missing vcpkg dependencies required by Catena on Windows:

```powershell
.\vcpkg install curl:x64-windows boost-system:x64-windows boost-date-time:x64-windows boost-url:x64-windows boost-asio:x64-windows asio:x64-windows gtest:x64-windows
```

4. Cleared stale configure cache when jwt-cpp path changed:

```cmd
cd /d <catena-root>\build
if exist cpp rmdir /s /q cpp
```

5. Reconfigured Catena with explicit LLVM tools, vcpkg triplet, and jwt-cpp install prefix (command in section above).

6. Installed codegen dependencies in the correct folder:

```powershell
cd <catena-root>\tools\codegen
npm install --yes
```

7. Built using Ninja:

```cmd
cd /d <catena-root>\build
cmake --build cpp
```

## Resolved Blockers

- [x] **`unittests/cpp/gRPC/tests/ServiceCredentials_test.cpp` fails on Windows**
  - Error: `setenv` / `unsetenv` are POSIX-only and are undefined on Windows.
  - Status: Resolved by cross-platform helper (`_putenv_s` on Windows, `setenv`/`unsetenv` on POSIX).

- [x] **Coverage link failure on Windows (`llvm_gcda_*` undefined)**
  - Error: link-time undefined symbols such as `llvm_gcda_start_file`, `llvm_gcda_emit_function`, `llvm_gcov_init`.
  - Status: Resolved by disabling coverage flags when `WIN32` is detected.

- [x] **Protobuf link mismatch on Windows clang-cl (`InternalSwap` / `memswap` unresolved)**
  - Error: unresolved protobuf symbols caused by clang `PROTOBUF_RESTRICT` mangling mismatch vs vcpkg-provided protobuf libs.
  - Status: Resolved with a Windows+Clang protobuf `port_def.inc` override that neutralizes `PROTOBUF_RESTRICT`.

## Known Remaining Blockers

- [ ] **`applocal.ps1` reports missing dump tool (non-blocking warning)**
  - Message: `Neither dumpbin, llvm-objdump nor objdump could be found. Can not take care of dll dependencies.`
  - Status: Build continues, but runtime DLL copy assistance is degraded.
  - Next fix: ensure `dumpbin` (VS tools) or `llvm-objdump` is on PATH during build.

- [ ] **Warning cleanup (non-blocking)**
  - Many warnings still present (`-Wdeprecated-literal-operator`, missing `override`, comment markers, deprecated `localtime` usage in `TimeNow.h`, conversion warnings in tests).
  - Status: Not currently blocking build completion.

## CMake / Source Code Patches Required

These patches have been implemented in this changeset:

- [x] **`sdks/cpp/cmake/PlatformConfig.cmake`** — Added `WIN32` branch: uses `clang -E -P -x c` as C preprocessor, uses `$ENV{USERPROFILE}` instead of `$ENV{HOME}`, suppresses `-Wno-braced-scalar-init` to APPLE only.
- [x] **`sdks/cpp/cmake/ExternalDependencies.cmake`** — Made Avahi (`pkg_check_modules`) conditional on `NOT WIN32`; fixed Node codegen install path (`tools/codegen`); made OpenAPI generation robust on Windows (prefer Git Bash, avoid WindowsApps bash, skip when `yq` is missing); made `PkgConfig` requirement conditional; added `find_package(jwt-cpp CONFIG REQUIRED)`.
- [x] **`sdks/cpp/common/CMakeLists.txt`** — Excluded `NmosNode.cpp` from sources on Windows (`if(NOT WIN32)`).
- [x] **`sdks/cpp/common/gRPC.cmake`** — Made Avahi include dirs and link libs conditional on `NOT WIN32`; linked `jwt-cpp::jwt-cpp` for `Authorizer` headers.
- [x] **`sdks/cpp/common/proto_interface.cmake`** — Linked `jwt-cpp::jwt-cpp` for `Authorizer` headers.
- [x] **`sdks/cpp/connections/REST/CMakeLists.txt`** — Made Avahi include dirs and link libs conditional on `NOT WIN32`.
- [x] **`sdks/cpp/connections/REST/include/SocketReader.h`** — Moved `EnumDecorator<RESTMethod>` specialization to the enclosing namespace to satisfy Clang on Windows.
- [x] **`sdks/cpp/connections/REST/examples/CMakeLists.txt`** — Disabled `discovery_REST` example on Windows (`NmosNode`/`netdb.h` is Linux-only).
- [x] **`unittests/cpp/common/CMakeLists.txt`** — Excluded `NmosNode_test.cpp` and all `--wrap` linker options on Windows. Replaced `pthread` with `Threads::Threads` via `find_package(Threads)`.
- [x] **`unittests/cpp/gRPC/CMakeLists.txt`** — Replaced `pthread` with `Threads::Threads`.
- [x] **`unittests/cpp/REST/CMakeLists.txt`** — Replaced `pthread` with `Threads::Threads`.
- [x] **All 17 example files** — Wrapped `signal(SIGKILL, ...)` in `#ifndef _WIN32` guards.
- [x] **`sdks/cpp/CMakeLists.txt`** — Added `BUILD_SHARED_LIBS` option and `CATENA_EXPORT` macro header generation for DLL support.
- [x] **`sdks/cpp/common/include/CatenaExport.h`** — Created new header providing `CATENA_API` visibility macro for shared library builds.
- [x] **`sdks/cpp/common/src/Logger.cpp`** — Replaced `localtime_r` with Windows-safe `localtime_s` under `_WIN32`.
- [x] **`smpte/tools/validator.js`** — Added fallback handling when generated schema artifacts are missing in Windows no-`yq` flow.

## Runtime Fixes Applied (gRPC `one_of_everything` on Windows)

These were required to get `build/cpp/connections/gRPC/examples/one_of_everything/one_of_everything.exe` running:

- [x] **`sdks/cpp/cmake/PlatformConfig.cmake`** — Explicitly enabled C++ exceptions on Windows builds with `/EHsc` for C++ sources. This fixes `cannot use 'try' with exceptions disabled` build failures under `clang-cl`.
- [x] **`tools/codegen/cpp/device.js`** — Changed generated device detail-level initialization from runtime string conversion (`DetailLevel("...")()`) to direct protobuf enum constants (`st2138::Device_DetailLevel_*`). This avoids Windows static initialization order issues that were causing startup/runtime crashes in `one_of_everything`.
- [x] **Rebuild required after generator change** — Rebuild `one_of_everything` so `device.one_of_everything.yaml.cpp` is regenerated with the enum-constant detail level.

## Known Limitations

- **NmosNode** is disabled on Windows (uses Linux-only Avahi mDNS). A future Windows implementation could use Bonjour SDK or Windows DNS-SD APIs.
- **Code coverage** (`--coverage`, gcovr, lcov) is Linux-only. Not applicable to Windows builds.
- **`__PRETTY_FUNCTION__`** (used in some REST code) — Clang supports this, but if switching to MSVC, `__FUNCSIG__` would be needed.
- Shared library builds (`.dll`) require decorating public APIs with `CATENA_API` macro from `CatenaExport.h` on each exported symbol. This is scaffolded but not yet applied to individual classes/functions.

## Verification

- [x] Run the CMake configure step — succeeded.
- [x] Run `ninja` — build completed successfully.
- [ ] Run `ctest` — unit tests should pass (NmosNode tests excluded on Windows).
- [x] Verify produced `.dll` files for `one_of_everything` in the output directory.
