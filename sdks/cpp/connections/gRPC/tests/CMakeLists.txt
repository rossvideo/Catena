cmake_minimum_required(VERSION 3.20)

set(This gRPC_UnitTests)

find_package(GTest REQUIRED)
include_directories(../src)
include_directories(../include)
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
    UpdateSubscriptions_tests.cpp
    ../src/UpdateSubscriptions.cpp
    ../src/ServiceImpl.cpp
    ../src/AddLanguage.cpp
    ../src/LanguagePackRequest.cpp
    ../src/MultiSetValue.cpp
    ../src/SetValue.cpp
    ../src/SubscriptionManager.cpp 
    ../src/GetPopulatedSlots.cpp  
    ../src/GetValue.cpp           
    ../src/Connect.cpp            
    ../src/DeviceRequest.cpp      
    ../src/ExternalObjectRequest.cpp  
    ../src/BasicParamInfoRequest.cpp  
    ../src/GetParam.cpp           
    ../src/ExecuteCommand.cpp     
    ../src/ListLanguages.cpp 
    
)
set(TESTS
    UpdateSubscriptions_tests
)
add_executable(${This} ${SOURCES})

target_link_libraries(${This} PUBLIC
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    ${common_lib}
    pthread
    PRIVATE
    ${GTEST_BOTH_LIBRARIES}
    GTest::GTest
    GTest::Main
    GTest::gmock
    GTest::gmock_main
    catena_grpc_common
)

add_test(
    NAME ${TESTS}
    COMMAND ${This}
)
