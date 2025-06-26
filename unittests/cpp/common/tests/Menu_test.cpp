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
 * @brief This file is for testing the Menu.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/26
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// protobuf
#include <interface/device.pb.h>
#include <interface/service.grpc.pb.h>
#include <google/protobuf/util/json_util.h>

// common
#include "Menu.h"

using namespace catena::common;

class MenuTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Setup code if needed

    }

    void TearDown() override {
        // Cleanup code if needed
    }

    std::unique_ptr<Menu> menu_;
    std::vector<std::pair<std::string, std::string>> names = {
        {"en", "Name"},
        {"fr", "Name but in French"}
    };
    bool hidden = true;
    bool disabled = true;
    std::vector<std::string> paramOids_ = {
        "param1",
        "param2"
    };
    std::vector<std::string> commandOids_ {
        "command1",
        "command2"
    };
    std::vector<std::pair<std::string, std::string>> clientHints_ = {
        {"hint1", "This is a hint"},
        {"hint2", "This is another hint"}
    };
};

TEST_F(MenuTest, Menu_Create) {

}

TEST_F(MenuTest, Menu_Move) {
    
}

