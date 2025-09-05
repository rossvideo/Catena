
cmake_minimum_required(VERSION 3.20)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(mocks)
include_directories(${CATENA_UNITTESTS_DIR}/cpp/common/mocks)
include_directories(${CATENA_UNITTESTS_DIR}/cpp/common)

# List all test files in tests/
set(GRPC_TEST_FILES
    AddLanguage_test.cpp
    Connect_test.cpp
    DeviceRequest_test.cpp
    ExecuteCommand_test.cpp
    GetParam_test.cpp
    GetPopulatedSlots_test.cpp
    GetValue_test.cpp
    LanguagePackRequest_test.cpp
    ListLanguages_test.cpp
    MultiSetValue_test.cpp
    ParamInfoRequest_test.cpp
    ServiceImpl_test.cpp
    SetValue_test.cpp
    UpdateSubscriptions_test.cpp
    ICallData_test.cpp
)

foreach(test_file ${GRPC_TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(gRPC_${test_name} tests/${test_file})
    target_link_libraries(gRPC_${test_name} PRIVATE
        catena_grpc_common
        catena_connections_grpc
        pthread
        ${GTEST_BOTH_LIBRARIES}
        glog::glog
        GTest::GTest
        GTest::Main
        GTest::gmock
        GTest::gmock_main
    )
    add_test(NAME gRPC.${test_name} COMMAND gRPC_${test_name})
endforeach()
