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
 * @date 2025-06-24
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// mock classes
#include "MockDevice.h"

// REST
#include "ServiceImpl.h"

using namespace catena::common;
using namespace catena::REST;

class RESTServiceImplTests : public testing::Test {
  protected:
    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        // oldCout_ = std::cout.rdbuf(MockConsole_.rdbuf());

        service_ = std::make_unique<CatenaServiceImpl>(dm_, EOPath_, authzEnabled_, port_);
    }

    void TearDown() override {
        // std::cout.rdbuf(oldCout_);
    }

    std::unique_ptr<CatenaServiceImpl> service_ = nullptr;

    // Cout variables
    std::stringstream MockConsole_;
    std::streambuf* oldCout_;

    uint16_t port_ = 50050;
    bool authzEnabled_ = false;

    MockDevice dm_;
    std::string EOPath_ = "path/to/extenal/object";
};

TEST_F(RESTServiceImplTests, ServiceImpl_Create) {
    ASSERT_TRUE(service_);
    EXPECT_EQ(service_->authorizationEnabled(), authzEnabled_);
    EXPECT_EQ(service_->version(), "v1");
}

TEST_F(RESTServiceImplTests, ServiceImpl_RunAndShutdown) {
    std::thread run_thread([this]() {
        service_->run();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Allow some time for the server to start
    service_->Shutdown();
    run_thread.join();
}
