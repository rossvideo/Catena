cmake_minimum_required(VERSION 3.20)	

set(CATENA_UNITTESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory("common/" common/)
add_subdirectory("gRPC/" gRPC/)
add_subdirectory("REST/" REST/)