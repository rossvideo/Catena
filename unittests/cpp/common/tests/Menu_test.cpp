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

// Mock objects
#include "MockMenuGroup.h"

// common
#include "Menu.h"

using namespace catena::common;

class MenuTest : public ::testing::Test {
  protected:
    // Creates a Menu for testing.
    void SetUp() override {
        EXPECT_CALL(menuGroup_, addMenu(oid_, testing::_)).Times(1)
            .WillOnce(testing::Invoke([this](const std::string& oid, std::unique_ptr<IMenu> menu) {
                ASSERT_TRUE(menu) << "Menu should not be nullptr";
                menu_ = std::move(menu);
            }));
        Menu({names_[0],       names_[1]}, hidden_, disabled_,
             {paramOids_[0],   paramOids_[1]},
             {commandOids_[0], commandOids_[1]},
             {clientHints_[0], clientHints_[1]}, oid_, menuGroup_);
    }

    std::unique_ptr<IMenu> menu_;
    std::vector<std::pair<std::string, std::string>> names_ = {
        {"en", "Name"},
        {"fr", "Name but in French"}
    };
    bool hidden_ = true;
    bool disabled_ = true;
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
    std::string oid_ = "test_menu";
    MockMenuGroup menuGroup_;
};

/*
 * TEST 1 - Menu creation.
 */
TEST_F(MenuTest, Menu_Create) {
    EXPECT_TRUE(menu_) << "Failed to create Menu";
}

/*
 * TEST 2 - Menu constructor error handling.
 */
TEST_F(MenuTest, Menu_ErrCreate) {
    MockMenuGroup errMenuGroup;
    EXPECT_CALL(errMenuGroup, addMenu(oid_, testing::_)).Times(1)
        .WillOnce(testing::Throw(std::runtime_error("MenuGroup error")));
    EXPECT_THROW(Menu({}, hidden_, disabled_, {}, {}, {}, oid_, errMenuGroup), std::runtime_error);
}

/*
 * TEST 3 - Menu toProto serialization.
 */
TEST_F(MenuTest, Menu_ToProto) {
    // Calling toProto
    ::st2138::Menu protoMenu;
    menu_->toProto(protoMenu);
    // Checking results
    for (auto& [lang, name] : names_) {
        EXPECT_EQ(protoMenu.name().display_strings().at(lang), name);
    }
    EXPECT_EQ(protoMenu.hidden(), hidden_);
    EXPECT_EQ(protoMenu.disabled(), disabled_);
    for (auto& oid: paramOids_) {
        EXPECT_TRUE(std::find(protoMenu.param_oids().begin(), protoMenu.param_oids().end(), oid) != protoMenu.param_oids().end());
    }
    for (auto& oid: commandOids_) {
        EXPECT_TRUE(std::find(protoMenu.command_oids().begin(), protoMenu.command_oids().end(), oid) != protoMenu.command_oids().end());
    }
    for (auto& [key, value] : clientHints_) {
        EXPECT_EQ(protoMenu.client_hints().at(key), value);
    }
}
