# Copyright 2024 Ross Video Ltd
# Platform-specific configuration module for Catena C++ SDK

cmake_minimum_required(VERSION 3.20)

# Platform detection and setup
function(setup_platform_config)
    message(STATUS "Configuring for platform: ${CMAKE_SYSTEM_NAME}")
    
    # macOS specific setup
    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "14.5" CACHE STRING "Minimum OS X deployment version")
        set(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk" PARENT_SCOPE)
        set(CMAKE_OSX_FRAMEWORK_PATH "${CMAKE_OSX_SYSROOT}/System/Library/Frameworks" PARENT_SCOPE)
        
        # Set up gRPC/protobuf paths for macOS
        set(GRPC_LIB_CMAKE "$ENV{HOME}/.local/lib/cmake")
        set(CMAKE_PREFIX_PATH "${GRPC_LIB_CMAKE}/protobuf;${GRPC_LIB_CMAKE}/grpc;${GRPC_LIB_CMAKE}/grpc/modules;${GRPC_LIB_CMAKE}/absl;${GRPC_LIB_CMAKE}/utf8_range" PARENT_SCOPE)
        
        # C preprocessor setup for macOS
        set(CPP clang PARENT_SCOPE)
        set(cpp_opts -E -P -x c PARENT_SCOPE)
        
        # Disable braced scalar initialization warnings
        add_compile_options(-Wno-braced-scalar-init)
        
    elseif(WIN32)
        # Windows platform
        add_compile_definitions(NOMINMAX)

        # Use clang as C preprocessor (ships with LLVM toolchain)
        # Prefer deriving from configured C compiler path so PATH is not required.
        unset(CPP)
        if(DEFINED CMAKE_C_COMPILER AND EXISTS "${CMAKE_C_COMPILER}")
            get_filename_component(_compiler_dir "${CMAKE_C_COMPILER}" DIRECTORY)
            if(EXISTS "${_compiler_dir}/clang.exe")
                set(CPP "${_compiler_dir}/clang.exe")
            endif()
        endif()

        if(NOT CPP)
            find_program(CPP NAMES clang clang-cl)
        endif()

        if(NOT CPP)
            message(FATAL_ERROR "C preprocessor not found. Install LLVM/Clang, add it to PATH, or set CMAKE_C_COMPILER to a full LLVM path.")
        endif()
        set(CPP ${CPP} PARENT_SCOPE)
        set(cpp_opts -E -P -x c PARENT_SCOPE)

        # Disable braced scalar initialization and non-standard types with offsetof warnings
        add_compile_options(-Wno-braced-scalar-init -Wno-invalid-offsetof)

    else()
        # Linux/other platforms
        find_program(CPP cpp)
        if(NOT CPP)
            message(FATAL_ERROR "C preprocessor not found")
        endif()
        set(CPP ${CPP} PARENT_SCOPE)
        set(cpp_opts -P -C PARENT_SCOPE)
    endif()
    
    # Common include directory
    if(WIN32)
        if(DEFINED ENV{USERPROFILE})
            include_directories("$ENV{USERPROFILE}/.local/include")
        endif()
    else()
        include_directories("$ENV{HOME}/.local/include")
    endif()
    
    message(STATUS "Using C preprocessor: ${CPP}")
endfunction()

# Compiler and linker configuration
function(setup_compiler_config)
    # Export compile commands for IDEs (can be overridden by command line)
    if(NOT DEFINED CMAKE_EXPORT_COMPILE_COMMANDS)
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON PARENT_SCOPE)
    endif()
    
    # Coverage flags - only add if COVERAGE is enabled
    option(COVERAGE "Enable code coverage flags" OFF)
    if(COVERAGE AND WIN32)
        message(WARNING "COVERAGE is not supported on Windows; disabling coverage flags for this configure.")
        set(COVERAGE OFF)
    endif()
    set(COVERAGE ${COVERAGE} PARENT_SCOPE)
    if(COVERAGE)
        add_compile_options(--coverage)
        add_link_options(--coverage)
    endif(COVERAGE)
    
    # Set default max connections
    if(NOT DEFINED DEFAULT_MAX_CONNECTIONS)
        set(DEFAULT_MAX_CONNECTIONS 16 CACHE STRING "Default max connections")
    endif()
    add_compile_definitions(DEFAULT_MAX_CONNECTIONS=${DEFAULT_MAX_CONNECTIONS})

    # Ensure C++ exceptions are enabled for clang-cl/MSVC style builds on Windows.
    # This project uses try/catch and throw in core runtime code.
    if(WIN32)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/EHsc>)
    endif()
endfunction()