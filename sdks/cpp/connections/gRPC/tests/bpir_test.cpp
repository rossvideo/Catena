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
 * @file basic_param_info_request_test.cpp
 * @brief Unit tests for BasicParamInfoRequest
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-04-04
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <BasicParamInfoRequest.h>
#include <Device.h>
#include <IParam.h>
#include <Path.h>
#include <Authorization.h>
#include <grpcpp/grpcpp.h>
#include <ServiceImpl.h>

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
    
    // Add all the missing pure virtual methods from IParam
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

// Mock class for grpc::ServerAsyncWriter
class MockServerAsyncWriter {
public:
    MOCK_METHOD(void, Write, (const catena::BasicParamInfoResponse& msg, void* tag), ());
};

class MockCatenaServiceImpl : public CatenaServiceImpl {
public:
    // Mock constructor
    MockCatenaServiceImpl(grpc::ServerCompletionQueue* cq, Device& dm, std::string& EOPath, bool authorizationEnabled)
        : CatenaServiceImpl(cq, dm, EOPath, authorizationEnabled) {}

    MOCK_METHOD(void, registerItem, (const std::string& oid), ());
    MOCK_METHOD(bool, authorizationEnabled, (), (const));
    MOCK_METHOD(void*, getCompletionQueue, (), (const));
    MOCK_METHOD(void*, getServer, (), (const));
    MOCK_METHOD(MockServerAsyncWriter*, getWriter, (), (const));
};

// Test fixture for BasicParamInfoRequest tests
class BasicParamInfoRequestTest : public ::testing::Test {
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
        // Cleanup code that will be called after each test
        if (server) {
            server->Shutdown();
            server->Wait();
        }

        if (cq) {
            cq->Shutdown();
            void* ignored_tag;
            bool ignored_ok;
            while (cq->Next(&ignored_tag, &ignored_ok)) { }
        }
        
        if (service) {
            delete service;
            service = nullptr;
        }
    }

    std::unique_ptr<MockDevice> device;
    std::unique_ptr<MockParam> param;
    std::string eoPath;
    std::unique_ptr<grpc::ServerCompletionQueue> cq;
    std::unique_ptr<grpc::Server> server;
    MockCatenaServiceImpl* service = nullptr;
};

// Test the constructor
TEST_F(BasicParamInfoRequestTest, Constructor) {
    // Create a mock device
    bool ok = true;
    
    // Create the request
    // CatenaServiceImpl::BasicParamInfoRequest request(service, *device, ok);
    
    // Add assertions here
    // EXPECT_TRUE(...);
}

// Test the proceed method
TEST_F(BasicParamInfoRequestTest, Proceed) {
    // Create a mock device
    bool ok = true;
    
    // Create the request
    // CatenaServiceImpl::BasicParamInfoRequest request(service, *device, ok);
    
    // Set up expectations
    // EXPECT_CALL(...);
    
    // Call the proceed method
    // request.proceed(service, ok);
    
    // Add assertions here
    // EXPECT_TRUE(...);
}

// Test the addParamToResponses method
TEST_F(BasicParamInfoRequestTest, AddParamToResponses) {
    // Create a mock device
    bool ok = true;
    
    // Create the request
    // CatenaServiceImpl::BasicParamInfoRequest request(service, *device, ok);
    
    // Create a mock authorizer
    class MockAuthorizer {
    public:
        MOCK_METHOD(bool, readAuthz, (const IParam& param), (const));
        MOCK_METHOD(bool, writeAuthz, (const IParam& param), (const));
    };
    MockAuthorizer authz;
    
    // Set up expectations
    EXPECT_CALL(*param, getName()).WillOnce(testing::Return("testParam"));
    EXPECT_CALL(*param, getPath()).WillOnce(testing::Return("/test/path"));
    EXPECT_CALL(*param, getValue()).WillOnce(testing::Return("testValue"));
    EXPECT_CALL(*param, isArray()).WillOnce(testing::Return(false));
    
    // Call the addParamToResponses method
    // request.addParamToResponses(param.get(), authz);
    
    // Add assertions here
    // EXPECT_TRUE(...);
}

// Test the updateArrayLengths method
TEST_F(BasicParamInfoRequestTest, UpdateArrayLengths) {
    // Create a mock device
    bool ok = true;
    
    // Create the request
    // CatenaServiceImpl::BasicParamInfoRequest request(service, *device, ok);
    
    // Call the updateArrayLengths method
    // request.updateArrayLengths("testArray", 10);
    
    // Add assertions here
    // EXPECT_TRUE(...);
} 