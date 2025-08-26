
cmake_minimum_required(VERSION 3.20)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${CATENA_CPP_ROOT_DIR}/common/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(mocks)
include_directories(${CATENA_UNITTESTS_DIR}/cpp/common)
include_directories(${CATENA_UNITTESTS_DIR}/cpp/common/mocks)

# List all test files in tests/
set(REST_TEST_FILES
    SocketReader_test.cpp
    SocketWriter_test.cpp
    GetValue_test.cpp
    LanguagePack_test.cpp
    Languages_test.cpp
    GetPopulatedSlots_test.cpp
    MultiSetValue_test.cpp
    SetValue_test.cpp
    GetParam_test.cpp
    ParamInfoRequest_test.cpp
    Subscriptions_test.cpp
    Connect_test.cpp
    ExecuteCommand_test.cpp
    DeviceRequest_test.cpp
    ServiceImpl_test.cpp
    AssetRequest_test.cpp
)

foreach(test_file ${REST_TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(REST_${test_name} tests/${test_file})
    target_link_libraries(REST_${test_name} PRIVATE
        catena_proto_common
        catena_connections_REST
        pthread
        ${GTEST_BOTH_LIBRARIES}
        glog::glog
        GTest::GTest
        GTest::Main
        GTest::gmock
        GTest::gmock_main
    )
    add_test(NAME REST.${test_name} COMMAND REST_${test_name})
endforeach()
