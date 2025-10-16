cmake_minimum_required(VERSION 3.20)

message(STATUS "Processing unittests/cpp/CMakeLists.txt")

add_subdirectory("common/" common/)
add_subdirectory("gRPC/" gRPC/)
add_subdirectory("REST/" REST/)