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
    tests/SubscriptionManager_test.cpp
    tests/ParamVisitor_test.cpp
    tests/Authorizer_test.cpp
    tests/Connect_test.cpp
    tests/MenuGroup_test.cpp
    tests/Menu_test.cpp
    tests/StructInfo_test.cpp
    tests/LanguagePack_test.cpp
    tests/Path_test.cpp
    tests/PolyglotText_test.cpp
    tests/ChoiceConstraint_test.cpp
    tests/RangeConstraint_test.cpp
    tests/ParamDescriptor_test.cpp
    tests/Device_test.cpp
    tests/ConnectionQueue_test.cpp
    tests/ParamWithValue_test/ParamWithValue_test.cpp
    tests/ParamWithValue_test/ParamWithInt_test.cpp
    tests/ParamWithValue_test/ParamWithFloat_test.cpp
    tests/ParamWithValue_test/ParamWithString_test.cpp
    tests/ParamWithValue_test/ParamWithIntArray_test.cpp
    tests/ParamWithValue_test/ParamWithFloatArray_test.cpp
    tests/ParamWithValue_test/ParamWithStringArray_test.cpp
    tests/ParamWithValue_test/ParamWithStruct_test.cpp
    tests/ParamWithValue_test/ParamWithStructArray_test.cpp
    tests/ParamWithValue_test/ParamWithVariant_test.cpp
    tests/ParamWithValue_test/ParamWithVariantArray_test.cpp
)

add_executable(${This} ${SOURCES})

target_link_libraries(${This} PRIVATE
    ${GTEST_BOTH_LIBRARIES}
    glog::glog
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
