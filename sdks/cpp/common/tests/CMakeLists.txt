cmake_minimum_required(VERSION 3.20)

set(This common_unit_tests)

find_package(GTest REQUIRED)
include_directories(../src ../include)
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
    subscription_manager_test.cpp
    param_visitor_test.cpp
    Authorization_test.cpp
    Connect_test.cpp
)

add_executable(${This} ${SOURCES})

target_link_libraries(${This} PRIVATE
    ${GTEST_BOTH_LIBRARIES}
    GTest::GTest
    GTest::Main
    GTest::gmock
    GTest::gmock_main
    ${common_lib}
)

add_test(
    NAME common_unit_tests
    COMMAND ${This}
)
