/*
 * Copyright 2025 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief This file is for testing the ServiceImpl.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 2025/07/03
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// common
#include <Logger.h>

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// mock classes
#include "MockDevice.h"
#include "MockConnect.h"

// REST
#include "ServiceImpl.h"

using namespace catena::common;
using namespace catena::REST;

class RESTServiceImplTests : public testing::Test {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("RESTServiceImplTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
  
    /*
     * Redirects cout and creates out service for testing.
     */
    void SetUp() override {
        oldCout_ = std::cout.rdbuf(MockConsole_.rdbuf());
        EXPECT_CALL(dm_, slot()).WillRepeatedly(testing::Return(0));
        service_.reset(new CatenaServiceImpl({&dm_}, EOPath_, authzEnabled_, port_, 1));
    }

    /*
     * Restored cout.
     */
    void TearDown() override {
        std::cout.rdbuf(oldCout_);
    }

    /*
     * Helper function which makes a REST call to the service.
     */
    std::string makeCall(RESTMethod method, const std::string& endpoint) {
        // Creating client and connecting to the service port.
        boost::asio::io_context io_context;
        tcp::socket clientSocket(io_context);
        clientSocket.connect(tcp::endpoint(tcp::v4(), port_));
        // Compiling request.
        std::string request = "";
        request += RESTMethodMap().getForwardMap().at(method);
        request += " /st2138-api/" + service_->version() + endpoint + " HTTP/1.1\r\n\r\n";
        // Writing to service.
        boost::asio::write(clientSocket, boost::asio::buffer(request));
        // Reading and returning the response.
        boost::asio::streambuf buffer;
        boost::asio::read_until(clientSocket, buffer, "\r\n\r\n");
        std::istream response_stream(&buffer);
        return std::string(std::istreambuf_iterator<char>(response_stream), std::istreambuf_iterator<char>());
    }

    std::unique_ptr<CatenaServiceImpl> service_ = nullptr;

    // Cout variables
    std::stringstream MockConsole_;
    std::streambuf* oldCout_;

    uint16_t port_ = 50050;
    bool authzEnabled_ = false;
    std::string EOPath_ = "path/to/extenal/object";

    // We really don't care about uninteresting function errors here.
    testing::NiceMock<MockDevice> dm_;
};

/*
 * TEST 1 - Creating a REST CatenaServiceImpl.
 */
TEST_F(RESTServiceImplTests, ServiceImpl_Create) {
    ASSERT_TRUE(service_);
    EXPECT_EQ(service_->authorizationEnabled(), authzEnabled_);
    EXPECT_EQ(service_->version(), "v1");
    EXPECT_NO_THROW(service_->subscriptionManager());
    EXPECT_NO_THROW(service_->connectionQueue());
}

/*
 * TEST 2 - Creating a REST CatenaServiceImpl.
 */
TEST_F(RESTServiceImplTests, ServiceImpl_CreateDuplicateSlot) {
    MockDevice dm2;
    EXPECT_CALL(dm2, slot()).WillRepeatedly(testing::Return(0));
    // Creating a service with a duplicate slot.
    EXPECT_THROW(CatenaServiceImpl({&dm_, &dm2}, EOPath_, authzEnabled_, port_ + 2, 1), std::runtime_error)
        << "Creating a service with two devices sharing a slot should throw an error.";
}

/*
 * TEST 3 - Running and shutting down the REST CatenaServiceImpl.
 */
TEST_F(RESTServiceImplTests, ServiceImpl_RunAndShutdown) {
    // Starting the service.
    std::thread run_thread([this]() {
        service_->run();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow some time for the server to start
    // Shutting down the service.
    service_->Shutdown();
    run_thread.join();
}

/*
 * TEST 4 - Testing the service's router against all valid endpoints. 
 */
TEST_F(RESTServiceImplTests, ServiceImpl_Router) {
    // Starting the service.
    std::thread run_thread([this]() {
        service_->run();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow some time for the server to start    
    // A vector of each method and endpoint to test.
    std::vector<std::pair<RESTMethod, std::string>> routes = {
        {Method_GET,     "/health"       },
        {Method_OPTIONS, "/value"        },
        {Method_GET,     "/devices"      },
        {Method_GET,     ""              },
        {Method_POST,    "/command"      },
        {Method_GET,     "/asset"        },
        // {Method_POST,    "/asset"        },
        // {Method_PUT,     "/asset"        }, Not implemented atm
        // {Method_DELETE,  "/asset"        },
        {Method_GET,     "/param-info"   },
        {Method_GET,     "/value"        },
        {Method_PUT,     "/value"        },
        {Method_PUT,     "/values"       },
        {Method_GET,     "/subscriptions"},
        {Method_PUT,     "/subscriptions"},
        {Method_GET,     "/param"        },
        {Method_GET,     "/connect"      },
        {Method_GET,     "/language-pack"},
        {Method_POST,    "/language-pack"},
        {Method_DELETE,  "/language-pack"},
        {Method_PUT,     "/language-pack"},
        {Method_GET,     "/languages"    }
    };
    for (auto& [method, endpoint] : routes) {
        std::string response = makeCall(method, endpoint);
        ASSERT_TRUE(!response.empty())
            << "No response read from " << RESTMethodMap().getForwardMap().at(method) << endpoint;
        EXPECT_TRUE(!response.starts_with("HTTP/1.1 501 Not Implemented"))
            << "Failed to route " << RESTMethodMap().getForwardMap().at(method) << endpoint;
    }
    // Making sure router returns proper error on invalid method/endpoints.
    std::string response = makeCall(Method_NONE, "/does-not-exist");
    ASSERT_TRUE(!response.empty())
        << "No response read from NONE/Does not exist";
    EXPECT_TRUE(response.starts_with("HTTP/1.1 501 Not Implemented"))
        << "Router should fail to route NONE/does-not-exist";

    // Shutting down the service.
    service_->Shutdown();
    run_thread.join();
}
