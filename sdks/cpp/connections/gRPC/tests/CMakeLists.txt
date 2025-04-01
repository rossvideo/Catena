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
    #Connect_tests.cpp
    
    ../src/AddLanguage.cpp
    ../src/BasicParamInfoRequest.cpp
    ../src/Connect.cpp
    ../src/DeviceRequest.cpp
    ../src/ExecuteCommand.cpp
    ../src/ExternalObjectRequest.cpp
    ../src/GetParam.cpp
    ../src/GetPopulatedSlots.cpp
    ../src/GetValue.cpp
    ../src/LanguagePackRequest.cpp
    ../src/ListLanguages.cpp
    ../src/MultiSetValue.cpp
    ../src/PeerInfo.cpp
    ../src/ServiceImpl.cpp
    ../src/SetValue.cpp
    ../src/SharedFlags.cpp
    ../src/SubscriptionManager.cpp
    ../src/UpdateSubscriptions.cpp
    
)

add_executable(${This} ${SOURCES})

target_link_libraries(${This}
    PUBLIC
        ${common_lib}
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