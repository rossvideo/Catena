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
 * @brief A parent class for gRPC test fixtures.
 * @author christian.twarogn@rossvideo.com
 * @date 16/06/25
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// std
#include <string>

// protobuf
#include <interface/device.pb.h>
#include <interface/service.grpc.pb.h>
#include <google/protobuf/util/json_util.h>

// common
#include <Status.h>

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
using boost::asio::ip::tcp;

// gRPC
#include "MockServer.h"

using namespace catena::gRPC;

/*
 * GRPCTest class inherited by test fixtures to provide functions for
 * writing, reading, and verifying requests and responses.
 */
class GRPCTest  : public ::testing::Test {
  protected:
    /*
     * Sets up expectations for the creation of a new CallData obj.
     */
    void SetUp() override {
        mockServer.start();
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());
        // We can always assume that a new CallData obj is created.
        // Either from initialization or kProceed.
        mockServer.expNew();
    }

    /*
     * Restores cout after each test.
     */
    void TearDown() override {
        std::cout.rdbuf(oldCout);

        // Redirecting cout to a stringstream for testing.
        std::stringstream MockConsole;
        std::streambuf* oldCout = std::cout.rdbuf(MockConsole.rdbuf());
        // Destroying the server.
        //TODO: GET NUM TIMES MAYBE
        EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).WillRepeatedly(::testing::Invoke([this]() {
            delete this->mockServer.testCall;
            this->mockServer.testCall = nullptr;
        }));
        mockServer.shutdown();
        // Restoring cout
        std::cout.rdbuf(oldCout);
    }

    // Console variables
    std::stringstream MockConsole;
    std::streambuf* oldCout;
    // Client variables.
    grpc::ClientContext clientContext;
    bool done = false;
    std::condition_variable cv;
    std::mutex cv_mtx;
    std::unique_lock<std::mutex> lock{cv_mtx};
    grpc::Status outRc;
    // Expected variables
    catena::Empty expVal;
    grpc::Status expRc;

    MockServer mockServer;
};
