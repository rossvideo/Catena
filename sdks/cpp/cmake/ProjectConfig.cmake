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
function(setup_logging_config CATENA_ROOT_DIR)
    # Set default log directory - respect command line override
    if(NOT DEFINED GLOG_LOGGING_DIR)
        set(GLOG_LOGGING_DIR "${CATENA_ROOT_DIR}/logs")
        message(STATUS "Using default glog logging directory: ${GLOG_LOGGING_DIR}")
    else()
        # Expand tilde if present
        string(REGEX REPLACE "^~" "$ENV{HOME}" GLOG_LOGGING_DIR "${GLOG_LOGGING_DIR}")
        message(STATUS "Using specified glog logging directory: ${GLOG_LOGGING_DIR}")
    endif()
    
    # Create logs directory if it doesn't exist
    if(NOT EXISTS "${GLOG_LOGGING_DIR}")
        file(MAKE_DIRECTORY "${GLOG_LOGGING_DIR}")
        message(STATUS "Created logging directory: ${GLOG_LOGGING_DIR}")
    endif()
    
    add_compile_definitions(GLOG_LOGGING_DIR="${GLOG_LOGGING_DIR}")
endfunction()

# Parse version information from VERSION.txt
function(setup_version_info CATENA_CPP_ROOT_DIR)
    # Read and parse version string
    file(READ ${CATENA_CPP_ROOT_DIR}/VERSION.txt VERSION_STRING)
    string(STRIP ${VERSION_STRING} VERSION_STRING)
    
    # Extract version and timestamp
    string(FIND "${VERSION_STRING}" "-" SPLIT_AT REVERSE)
    string(SUBSTRING "${VERSION_STRING}" 0 ${SPLIT_AT} CATENA_CPP_VERSION)
    math(EXPR TIMESTAMP_START "${SPLIT_AT} + 1")
    string(SUBSTRING "${VERSION_STRING}" ${TIMESTAMP_START} -1 CATENA_CPP_TIMESTAMP)
    
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
        enable_testing()
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