# Copyright 2024 Ross Video Ltd  
# Project configuration and versioning for Catena C++ SDK

cmake_minimum_required(VERSION 3.20)

# Set up CMAKE_MODULE_PATH to include local cmake files
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# Set up core project variables
function(setup_project_directories)
    # Set up root directories
    get_filename_component(CATENA_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
    set(CATENA_CPP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    
    # Make available as cache variables so they're accessible in all subdirectories
    set(CATENA_ROOT_DIR ${CATENA_ROOT_DIR} CACHE PATH "Catena workspace root directory")
    set(CATENA_CPP_ROOT_DIR ${CATENA_CPP_ROOT_DIR} CACHE PATH "Catena C++ SDK root directory")
    
    message(STATUS "Catena root directory: ${CATENA_ROOT_DIR}")
    message(STATUS "Catena C++ root directory: ${CATENA_CPP_ROOT_DIR}")
endfunction()

# Setup logging configuration  
function(setup_logging_config)
    # Set default log directory - respect command line override
    if(DEFINED LOG_DIR)
        # Expand tilde if present
        string(REGEX REPLACE "^~" "$ENV{HOME}" LOG_DIR "${LOG_DIR}")
        message(STATUS "Using specified logging directory: ${LOG_DIR}")
    endif()

    #if default log directory not set, use a standard location
    if(NOT DEFINED LOG_DIR)
        set(LOG_DIR "${CATENA_ROOT_DIR}/logs")
        message(STATUS "Using default logging directory: ${LOG_DIR}")
    endif()

    add_compile_definitions(LOG_DIR="${LOG_DIR}")
endfunction()

# Parse version information from VERSION.txt
function(setup_version_info CATENA_CPP_ROOT_DIR)
    # Read and parse version string
    file(READ ${CATENA_CPP_ROOT_DIR}/../../VERSION.txt VERSION_STRING)
    string(STRIP ${VERSION_STRING} VERSION_STRING)

    # Expected format: v<major>.<minor>.<patch>
    if(NOT VERSION_STRING MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+$")
        message(FATAL_ERROR "Invalid VERSION.txt format: '${VERSION_STRING}'. Expected format: v<major>.<minor>.<patch>")
    endif()
    set(CATENA_CPP_VERSION ${VERSION_STRING})

    # Timestamp comes from the last commit that changed VERSION.txt.
    execute_process(
        COMMAND git -C ${CATENA_CPP_ROOT_DIR}/../.. log -1 --format=%ct -- VERSION.txt
        OUTPUT_VARIABLE CATENA_CPP_TIMESTAMP
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE VERSION_GIT_RESULT
        ERROR_QUIET
    )
    if(NOT VERSION_GIT_RESULT EQUAL 0 OR "${CATENA_CPP_TIMESTAMP}" STREQUAL "")
        message(WARNING "Could not read VERSION.txt git history timestamp; using current UTC build time.")
        string(TIMESTAMP CATENA_CPP_TIMESTAMP "%Y%m%d%H%M%S" UTC)
    endif()
    
    # Set compile definitions and display info
    add_compile_definitions(CATENA_CPP_VERSION=${CATENA_CPP_VERSION})
    message(STATUS "Catena C++ SDK version: ${CATENA_CPP_VERSION}")
    message(STATUS "Catena C++ SDK timestamp: ${CATENA_CPP_TIMESTAMP}")
    
    # Make available to parent scope
    set(CATENA_CPP_VERSION ${CATENA_CPP_VERSION} PARENT_SCOPE)
    set(CATENA_CPP_TIMESTAMP ${CATENA_CPP_TIMESTAMP} PARENT_SCOPE)
endfunction()

# Setup testing configuration
function(setup_testing_config)
    option(UNIT_TESTING "Build the unit tests" ON)
    
    if(UNIT_TESTING)
        message(STATUS "Unit testing enabled")
    else()
        message(STATUS "Unit testing disabled")
    endif()
endfunction()

# Setup build options
function(setup_build_options)
    option(ONLY_DOCS "Only build the documentation" OFF)
    
    # Make options available to parent scope
    set(ONLY_DOCS ${ONLY_DOCS} PARENT_SCOPE)
    set(UNIT_TESTING ${UNIT_TESTING} PARENT_SCOPE)
endfunction()

# Main project configuration function
function(configure_catena_project)
    setup_project_directories()
    setup_logging_config(${CATENA_ROOT_DIR})
    setup_version_info(${CATENA_CPP_ROOT_DIR})
    setup_testing_config()
    setup_build_options()
    
    message(STATUS "Project configuration complete")
endfunction()