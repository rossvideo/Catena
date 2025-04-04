cmake_minimum_required(VERSION 3.20)

set(This gRPCUnitTests)

find_package(GTest REQUIRED)
include_directories(../src)
include_directories(../include)
include_directories(${GTEST_INCLUDE_DIRS})

# Set up link libraries
set(grpc_lib catena_connections_grpc)

set(SOURCES 
    grpc_test.cpp
    bpir_test.cpp
    device_request_test.cpp
    update_subscriptions_test.cpp
    
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
    ../src/UpdateSubscriptions.cpp
)

add_executable(${This} ${SOURCES})

target_link_libraries(${This}
    PUBLIC
        ${grpc_lib}
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
    NAME ${This}
    COMMAND ${This}
) 