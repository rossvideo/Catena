cmake_minimum_required(VERSION 3.20)	

set(This REST_UnitTests)	

find_package(GTest REQUIRED)	
include_directories(../src)	

include_directories(../include)	
include_directories(${GTEST_INCLUDE_DIRS})	

set(SOURCES 	
    SocketReader_test.cpp
    SocketWriter_test.cpp    
    GetValue_test.cpp
    ListLanguages_test.cpp

    ../src/SocketReader.cpp
    ../src/SocketWriter.cpp
    ../src/controllers/GetValue.cpp
    ../src/controllers/ListLanguages.cpp
)	

add_executable(${This} ${SOURCES})	

target_link_libraries(${This}	
    PUBLIC	
        catena_proto_common
        catena_connections_REST
        pthread	
    PRIVATE	
        ${GTEST_BOTH_LIBRARIES}	
        GTest::GTest	
        GTest::Main	
        GTest::gmock	
        GTest::gmock_main	
        catena_proto_common	
        pthread	
)	

add_test(	
    NAME REST_Unit_tests	
    COMMAND ${This}	
)	
