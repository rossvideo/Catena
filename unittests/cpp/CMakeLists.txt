cmake_minimum_required(VERSION 3.20)

message(STATUS "Processing unittests/cpp/CMakeLists.txt")

add_compile_definitions(UNITTEST_LOG_DIR="${CATENA_UNITTESTS_DIR}/logs")

add_subdirectory("common/" common/)
add_subdirectory("gRPC/" gRPC/)
add_subdirectory("REST/" REST/)