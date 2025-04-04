// Copyright 2025 Ross Video Ltd
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/**
 * @file device_request_test.cpp
 * @brief Unit tests for DeviceRequest
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-04-04
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <DeviceRequest.h>
#include <Device.h>
#include <IParam.h>
#include <Path.h>
#include <Authorization.h>
#include <grpcpp/grpcpp.h>
#include <memory>

using namespace grpc;
using namespace catena::common;
using ::testing::Return;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::AnyNumber;

// Mock for exception_with_status
class MockExceptionWithStatus {
public:
    MockExceptionWithStatus() {}
    MockExceptionWithStatus(const std::string& message) {}
    MockExceptionWithStatus(const std::string& message, int code) {}
    MockExceptionWithStatus(const MockExceptionWithStatus& other) {}
    MockExceptionWithStatus& operator=(const MockExceptionWithStatus& other) { return *this; }
    ~MockExceptionWithStatus() {}
};

// Mock classes for testing
class MockDevice : public Device {
public:
    MOCK_METHOD(IParam*, getParam, (const std::string& path), ());
    MOCK_METHOD(bool, setValue, (const std::string& path, const std::string& value), ());
    MOCK_METHOD(std::string, getValue, (const std::string& path), ());
    MOCK_METHOD(void, traverseParams, (IParam* param, const std::string& path, catena::common::IParamVisitor& visitor), ());
};

class MockParam : public IParam {
public:
    MOCK_METHOD(std::string, getName, (), (const));
    MOCK_METHOD(std::string, getPath, (), (const));
    MOCK_METHOD(std::string, getValue, (), (const));
    MOCK_METHOD(bool, setValue, (const std::string& value), ());
    MOCK_METHOD(bool, isArray, (), (const));
    MOCK_METHOD(uint32_t, getArrayLength, (), (const));
    MOCK_METHOD(std::unique_ptr<IParam>, copy, (), (const));
    // Implement all pure virtual methods from IParam
    MOCK_METHOD(catena::exception_with_status, toProto, (catena::Value& dst, catena::common::Authorizer& authz), (const));
    MOCK_METHOD(catena::exception_with_status, fromProto, (const catena::Value& src, catena::common::Authorizer& authz), ());
    MOCK_METHOD(catena::exception_with_status, toProto, (catena::Param& param, catena::common::Authorizer& authz), (const));
    MOCK_METHOD(catena::exception_with_status, toProto, (catena::BasicParamInfoResponse& paramInfo, catena::common::Authorizer& authz), (const));
    MOCK_METHOD(IParam::ParamType, type, (), (const));
    MOCK_METHOD(const std::string&, getOid, (), (const));
    MOCK_METHOD(void, setOid, (const std::string& oid), ());
    MOCK_METHOD(bool, readOnly, (), (const));
    MOCK_METHOD(void, readOnly, (bool flag), ());
    MOCK_METHOD(std::unique_ptr<IParam>, getParam, (Path& oid, catena::common::Authorizer& authz, catena::exception_with_status& status), ());
    MOCK_METHOD(uint32_t, size, (), (const));
    MOCK_METHOD(std::unique_ptr<IParam>, addBack, (catena::common::Authorizer& authz, catena::exception_with_status& status), ());
    MOCK_METHOD(catena::exception_with_status, popBack, (catena::common::Authorizer& authz), ());
    MOCK_METHOD(const catena::common::IConstraint*, getConstraint, (), (const));
    MOCK_METHOD(const std::string&, getScope, (), (const));
    MOCK_METHOD(void, defineCommand, (std::function<catena::CommandResponse(catena::Value)> command), ());
    MOCK_METHOD(catena::CommandResponse, executeCommand, (const catena::Value& value), (const));
    MOCK_METHOD(const catena::common::ParamDescriptor&, getDescriptor, (), (const));
    MOCK_METHOD(bool, isArrayType, (), (const));
    MOCK_METHOD(bool, validateSetValue, (const catena::Value& value, Path::Index index, catena::common::Authorizer& authz, catena::exception_with_status& ans), ());
    MOCK_METHOD(void, resetValidate, (), ());
};

class MockCatenaServiceImpl : public CatenaServiceImpl {
public:
    // Constructor matching the base class
    MockCatenaServiceImpl(grpc::ServerCompletionQueue* cq, Device& dm, std::string& EOPath, bool authorizationEnabled)
        : CatenaServiceImpl(cq, dm, EOPath, authorizationEnabled) {}

    // Add necessary methods
    MOCK_METHOD(void, registerItem, (void* item), ());
    MOCK_METHOD(void, deregisterItem, (void* item), ());
    MOCK_METHOD(void, RequestDeviceRequest, (grpc::ServerContext* context, 
                                           const catena::DeviceRequestPayload* request,
                                           grpc::ServerAsyncWriter<catena::DeviceComponent>* writer,
                                           grpc::ServerCompletionQueue* cq,
                                           grpc::ServerCompletionQueue* notify_cq,
                                           void* tag), ());
    MOCK_METHOD(bool, authorizationEnabled, (), (const));
    MOCK_METHOD(std::string, getJWSToken, (), (const));
    
    virtual ~MockCatenaServiceImpl() = default;
};

// Test fixture for DeviceRequest tests
class DeviceRequestTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code that will be called before each test
        device = std::make_unique<MockDevice>();
        param = std::make_unique<MockParam>();
        
        eoPath = "/test/path";
        bool authEnabled = false;

        grpc::ServerBuilder builder;

        // Create a completion queue
        cq = builder.AddCompletionQueue();

        // Create the service instance
        service = new MockCatenaServiceImpl(cq.get(), *device, eoPath, authEnabled);

        // Register the service with the server
        builder.RegisterService(service);

        // Build and start the server
        server = builder.BuildAndStart();

        ASSERT_TRUE(server != nullptr) << "Failed to start gRPC server.";
    }

    void TearDown() override {
        //shutdown the server
        if (server){
            server->Shutdown();
            server->Wait(); 
        }

        if (cq){
            cq->Shutdown();
            void* ignored_tag;
            bool ignored_ok;
            while (cq->Next(&ignored_tag, &ignored_ok)) { }
        }
        if (service){
            delete service;
            service = nullptr;
        }
    }

    std::string eoPath;
    std::unique_ptr<grpc::ServerCompletionQueue> cq;
    std::unique_ptr<grpc::Server> server;
    std::unique_ptr<MockDevice> device;
    std::unique_ptr<MockParam> param;
    MockCatenaServiceImpl* service = nullptr;
};

// Test the constructor
TEST_F(DeviceRequestTest, Constructor) {
    // Create a mock device
    bool ok = true;
    
    // Set up expectations for the service
    EXPECT_CALL(*service, registerItem(::testing::_)).Times(1);
    EXPECT_CALL(*service, RequestDeviceRequest(::testing::_, ::testing::_, ::testing::_, 
                                             ::testing::_, ::testing::_, ::testing::_)).Times(1);
    
    // Create the request
    CatenaServiceImpl::DeviceRequest request(service, *device, ok);
    
    // Add assertions here
    SUCCEED();
}

// Test the proceed method
TEST_F(DeviceRequestTest, Proceed) {
    // Create a mock device
    bool ok = true;
    
    // Set up expectations
    EXPECT_CALL(*service, registerItem(::testing::_)).Times(1);
    EXPECT_CALL(*service, RequestDeviceRequest(::testing::_, ::testing::_, ::testing::_, 
                                             ::testing::_, ::testing::_, ::testing::_)).Times(1);
    
    // Create the request
    CatenaServiceImpl::DeviceRequest request(service, *device, ok);
    
    // Call the proceed method
    request.proceed(service, ok);
    
    // Add assertions here
    SUCCEED();
}

// Test the processDeviceRequest method
TEST_F(DeviceRequestTest, ProcessDeviceRequest) {
    // Create a mock device
    bool ok = true;
    
    // Set up expectations
    EXPECT_CALL(*service, registerItem(::testing::_)).Times(1);
    EXPECT_CALL(*service, authorizationEnabled()).WillOnce(::testing::Return(false));
    
    // Create the request
    CatenaServiceImpl::DeviceRequest request(service, *device, ok);
    
    // Call the proceed method which will eventually call processDeviceRequest
    request.proceed(service, ok);
    
    // Add assertions here
    SUCCEED();
} 