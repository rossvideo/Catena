cmake_minimum_required(VERSION 3.10)
project(Tests)

# Add Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add the source directory for utils.h
include_directories(../include)


# Add the test executable
add_executable(utils_test utils_test.cpp)

# Link Google Test and pthread
target_link_libraries(utils_test ${GTEST_LIBRARIES} pthread)