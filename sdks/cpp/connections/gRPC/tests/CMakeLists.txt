cmake_minimum_required(VERSION 3.20)	

set(This gRPC_UnitTests)	

find_package(GTest REQUIRED)	
include_directories(../src)	

include_directories(../include)	
include_directories(${GTEST_INCLUDE_DIRS})	
include_directories(../../common/tests)

set(SOURCES 	
    GetPopulatedSlots_test.cpp
    GetParam_test.cpp
    GetValue_test.cpp
    MultiSetValue_test.cpp
    SetValue_test.cpp
    ListLanguages_test.cpp
    LanguagePackRequest_test.cpp
    AddLanguage_test.cpp
    DeviceRequest_test.cpp
    ExecuteCommand_test.cpp
)	

add_executable(${This} ${SOURCES})	

target_link_libraries(${This}	
    PUBLIC	
        catena_grpc_common
        catena_connections_grpc
        pthread	
    PRIVATE	
        ${GTEST_BOTH_LIBRARIES}	
        GTest::GTest	
        GTest::Main	
        GTest::gmock	
        GTest::gmock_main	
        catena_grpc_common	
        pthread	
)	

add_test(	
    NAME gRPC_Unit_tests
    COMMAND ${This}	
)	
