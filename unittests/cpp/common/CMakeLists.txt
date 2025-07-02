cmake_minimum_required(VERSION 3.20)

set(This common_unit_tests)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CATENA_UNITTESTS_DIR}/cpp/REST/mocks)
include_directories(mocks)

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
    tests/utils_test.cpp
    tests/subscription_manager_test.cpp
    tests/param_visitor_test.cpp
    tests/Authorization_test.cpp
    tests/Connect_test.cpp
    tests/LanguagePack_test.cpp
    tests/PolyglotText_test.cpp
    tests/PicklistConstraint_test.cpp
    tests/NamedChoiceConstraint_test.cpp
    tests/RangeConstraint_test.cpp
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
