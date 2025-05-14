cmake_minimum_required(VERSION 3.20)	

set(This REST_UnitTests)	

find_package(GTest REQUIRED)	
include_directories(../src)	

include_directories(../include)	
include_directories(${GTEST_INCLUDE_DIRS})	
include_directories(../../common/tests)

set(SOURCES 	
    SocketReader_test.cpp
    SocketWriter_test.cpp    
    GetValue_test.cpp
    AddLanguage_test.cpp
    LanguagePackRequest_test.cpp
    ListLanguages_test.cpp
    MultiSetValue_test.cpp
    SetValue_test.cpp

    ../src/SocketReader.cpp
    ../src/SocketWriter.cpp
    ../src/controllers/GetValue.cpp
    ../src/controllers/AddLanguage.cpp
    ../src/controllers/LanguagePackRequest.cpp
    ../src/controllers/ListLanguages.cpp
    ../src/controllers/MultiSetValue.cpp
    ../src/controllers/SetValue.cpp
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
