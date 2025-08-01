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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
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
 * @brief This file is for testing the ConnectionQueue.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/07/24
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <rpc/ConnectionQueue.h>
#include "MockConnect.h"

// gtest
#include <gtest/gtest.h>

// common
#include <Logger.h>

using namespace catena::common;

// Test fixture for ConnectionQueue tests
class ConnectionQueueTests : public ::testing::Test {
protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("ConnectionQueueTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
};

/**
 * TEST 1 - Testing ConnectionQueue registerConnection and deregisterConnection.
 */
TEST_F(ConnectionQueueTests, ConnectionQueue_ManageConnections) {
    // Initializing connectionQueue with maxConnections = 1.
    ConnectionQueue connectionQueue{1};
    // Mocking 2 connections with A < B.
    MockConnect connectionA, connectionB;
    bool shutdownA = false;
    bool shutdownB = false;
    EXPECT_CALL(connectionA, shutdown()).WillRepeatedly(testing::Invoke([&shutdownA]() { shutdownA = true; }));
    EXPECT_CALL(connectionA, lessThan(testing::_)).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(connectionB, shutdown()).WillRepeatedly(testing::Invoke([&shutdownB]() { shutdownB = true; }));
    EXPECT_CALL(connectionB, lessThan(testing::_)).WillRepeatedly(testing::Return(false));
    // Registering connection A.
    EXPECT_TRUE(connectionQueue.registerConnection(&connectionA)) << "ConnectionQueue should be able to register a connection.";
    // Setting connection B to higher prioirity and registering.
    EXPECT_TRUE(connectionQueue.registerConnection(&connectionB)) << "ConnectionQueue should be able to register a higher priority connection.";
    EXPECT_TRUE(shutdownA) << "Lower priority connections should be shutdown when a higher priority connection is registered.";
    connectionQueue.deregisterConnection(&connectionA);
    // Trying to re-add connection A should fail.
    EXPECT_FALSE(connectionQueue.registerConnection(&connectionA)) << "ConnectionQueue should not be able to register a lower priority connection";
    EXPECT_FALSE(shutdownB) << "Higher priority connection should not be shutdown when a lower priority connection tries to connect.";
    connectionQueue.deregisterConnection(&connectionB);
}

