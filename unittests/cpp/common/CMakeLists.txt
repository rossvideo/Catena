
cmake_minimum_required(VERSION 3.20)

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

# List all test files in tests/ (excluding ParamWithValue_test subdir)
set(TEST_FILES
    utils_test.cpp
    SubscriptionManager_test.cpp
    ParamVisitor_test.cpp
    Authorizer_test.cpp
    Connect_test.cpp
    MenuGroup_test.cpp
    Menu_test.cpp
    StructInfo_test.cpp
    LanguagePack_test.cpp
    Path_test.cpp
    PolyglotText_test.cpp
    ChoiceConstraint_test.cpp
    RangeConstraint_test.cpp
    ParamDescriptor_test.cpp
    Device_test.cpp
    ConnectionQueue_test.cpp
)

foreach(test_file ${TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(common_${test_name} tests/${test_file})
    target_link_libraries(common_${test_name} PRIVATE
        ${GTEST_BOTH_LIBRARIES}
        glog::glog
        GTest::GTest
        GTest::Main
        GTest::gmock
        GTest::gmock_main
        ${common_lib}
    )
    add_test(NAME common.${test_name} COMMAND common_${test_name})
endforeach()

# ParamWithValue_test subdir: one executable for all files
file(GLOB PARAMWITHVALUE_TESTS "tests/ParamWithValue_test/*.cpp")
add_executable(common_ParamWithValue ${PARAMWITHVALUE_TESTS})
target_link_libraries(common_ParamWithValue PRIVATE
    ${GTEST_BOTH_LIBRARIES}
    glog::glog
    GTest::GTest
    GTest::Main
    GTest::gmock
    GTest::gmock_main
    ${common_lib}
)
add_test(NAME common.ParamWithValue COMMAND common_ParamWithValue)
