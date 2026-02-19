// Copyright 2024 Ross Video Ltd
//
// DLL export/import macro for Windows shared library builds.
//
// Usage:
//   class CATENA_API MyClass { ... };
//   CATENA_API void myFunction();
//
// When building as a shared library (DLL) on Windows, CATENA_SHARED is defined
// by CMake. The exporting library defines CATENA_EXPORTS during its build.
// Consumers of the DLL will see CATENA_API as __declspec(dllimport).
//
// On non-Windows platforms or static builds, CATENA_API is empty.
//

#pragma once

#if defined(_WIN32) && defined(CATENA_SHARED)
    #if defined(CATENA_EXPORTS)
        #define CATENA_API __declspec(dllexport)
    #else
        #define CATENA_API __declspec(dllimport)
    #endif
#else
    #define CATENA_API
#endif
