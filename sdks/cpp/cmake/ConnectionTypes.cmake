# Copyright 2024 Ross Video Ltd
# Connection type management for Catena C++ SDK

cmake_minimum_required(VERSION 3.20)

# Handle connection types - support both semicolon and space-separated lists
if(CONNECTIONS)
    # Convert semicolon-separated string to list if needed
    string(REPLACE ";" " " CONNECTIONS_DISPLAY "${CONNECTIONS}")
    message(STATUS "Processing connections: ${CONNECTIONS_DISPLAY}")
    
    # Ensure CONNECTIONS is a proper list
    if(NOT CONNECTIONS MATCHES ";")
        string(REPLACE " " ";" CONNECTIONS "${CONNECTIONS}")
    endif()
endif()

# Define allowed connection types
set(ALLOWED_CONNECTIONS "gRPC;REST")

# Validate connection types
foreach(conn ${CONNECTIONS})
    list(FIND ALLOWED_CONNECTIONS "${conn}" _IDX)
    if(${_IDX} EQUAL -1)
        message(FATAL_ERROR "Invalid connection type: '${conn}'. Allowed types: ${ALLOWED_CONNECTIONS}")
    endif()
endforeach()

# Set up connection flags
message(STATUS "Enabled connections: ${CONNECTIONS}")

list(FIND CONNECTIONS "gRPC" _gRPC)
list(FIND CONNECTIONS "REST" _REST)
list(FIND CONNECTIONS "WSS" _WSS)

# Set connection flags
if(${_gRPC} GREATER -1)
    message(STATUS "gRPC connection enabled")
    set(gRPC_enabled TRUE)
endif()

if(${_REST} GREATER -1)  
    message(STATUS "REST connection enabled")
    set(REST_enabled TRUE)
endif()

if(${_WSS} GREATER -1)
    message(STATUS "WSS connection enabled")
    set(WSS_enabled TRUE)
endif()

# Ensure at least one connection type is enabled
if(NOT (${_gRPC} GREATER -1 OR ${_REST} GREATER -1 OR ${_WSS} GREATER -1))
    message(FATAL_ERROR "At least one connection type must be specified")
endif()

# Proto file configuration
set(relative_interface "../../smpte/interface/proto/")
cmake_path(ABSOLUTE_PATH relative_interface OUTPUT_VARIABLE interface)
cmake_path(SET catena_interface_dir NORMALIZE ${interface})

# Proto file stems
set(interface_proto_stems 
    "language"
    "param"
    "externalobject" 
    "constraint"
    "device"
    "menu"
)

# Setup gRPC targets if enabled
if(gRPC_enabled)
    include(${CMAKE_CURRENT_LIST_DIR}/../gRPC.cmake)
    set_up_gRPC_targets()
    message(STATUS "gRPC targets configured")
endif()

# Setup REST targets if enabled
if(REST_enabled)
    include(proto_interface)
    set_up_proto_targets()
    message(STATUS "REST targets configured")
endif()