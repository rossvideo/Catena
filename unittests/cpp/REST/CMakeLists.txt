cmake_minimum_required(VERSION 3.20)	

set(This REST_unit_tests)	

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

include_directories(${CATENA_CPP_ROOT_DIR}/common/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(mocks)
include_directories(${CATENA_UNITTESTS_DIR}/cpp/common)
include_directories(${CATENA_UNITTESTS_DIR}/cpp/common/mocks)

set(SOURCES
    tests/SocketReader_test.cpp
    tests/SocketWriter_test.cpp
    tests/GetValue_test.cpp
    tests/LanguagePack_test.cpp
    tests/Languages_test.cpp
    tests/GetPopulatedSlots_test.cpp
    tests/MultiSetValue_test.cpp
    tests/SetValue_test.cpp
    tests/GetParam_test.cpp
    tests/ParamInfoRequest_test.cpp
    tests/Subscriptions_test.cpp
    tests/Connect_test.cpp
    tests/ExecuteCommand_test.cpp
    tests/DeviceRequest_test.cpp
    tests/ServiceImpl_test.cpp
)	

add_executable(${This} ${SOURCES})	

target_link_libraries(${This}	
    PUBLIC
        catena_proto_common
        catena_connections_REST
        pthread	
    PRIVATE	
        ${GTEST_BOTH_LIBRARIES}
        glog::glog
        GTest::GTest	
        GTest::Main	
        GTest::gmock	
        GTest::gmock_main	
        catena_proto_common	
        pthread	
)	

add_test(	
    NAME REST_unit_tests	
    COMMAND ${This}	
)	
