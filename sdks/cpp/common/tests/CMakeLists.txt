cmake_minimum_required(VERSION 3.20)

set(This common_unit_tests)

find_package(GTest REQUIRED)
include_directories(../src)
include_directories(../include)
include_directories(${GTEST_INCLUDE_DIRS})

# set up link libs, prefer gRPC if enabled
set(common_lib)
if (gRPC_enabled)
    set(common_lib catena_grpc_common)
else()
    if(REST_enabled)
        set(common_lib catena_proto_common)
    else()
        message(FATAL_ERROR "No connection type enabled")
    endif(REST_enabled)
endif(gRPC_enabled)

set(SOURCES 
    utils_test.cpp
)

add_executable(${This} ${SOURCES})

target_link_libraries(${This} PUBLIC
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    ${common_lib}
    GTest::gmock
    GTest::gmock_main
)

add_test(
    NAME common_unit_tests
    COMMAND ${This}
)
