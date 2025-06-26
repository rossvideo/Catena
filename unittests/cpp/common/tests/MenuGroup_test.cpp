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
 * @brief This file is for testing the MenuGroup.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/26
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Mock objects
#include <mocks/MockDevice.h>
#include <mocks/MockMenu.h>

// protobuf
#include <interface/device.pb.h>
#include <interface/service.grpc.pb.h>
#include <google/protobuf/util/json_util.h>

// common
#include "MenuGroup.h"

using namespace catena::common;

class MenuGroupTest : public ::testing::Test {
  protected:
    void SetUp() override {
        EXPECT_CALL(dm_, addItem(oid_, testing::An<IMenuGroup*>())).Times(1).WillOnce(
            testing::Invoke([](const std::string &key, IMenuGroup *item){
                EXPECT_TRUE(item) << "No item passed into dm.addItem()";
            }));
        menuGroup_.reset(new MenuGroup(oid_, {name_[0], name_[1]}, dm_));
    }

    void TearDown() override {
        // Cleanup code if needed
    }

    std::unique_ptr<MenuGroup> menuGroup_;

    std::string oid_ = "menu_group";
    std::vector<std::pair<std::string, std::string>> name_ = {
        {"en", "Menu Group"},
        {"fr", "Groupe de menus"}
    };
    MockDevice dm_;
    
};

TEST_F(MenuGroupTest, MenuGroup_Create) {
    EXPECT_TRUE(menuGroup_) << "Failed to create MenuGroup";
}

TEST_F(MenuGroupTest, MenuGroup_addMenu) {
    // Adding two menus.
    std::unordered_map<std::string, IMenu*> menus;
    menus["menu1"] = nullptr;
    menus["menu2"] = nullptr;
    for (auto& [oid, menuRef] : menus) {
        std::unique_ptr<MockMenu> menu = std::make_unique<MockMenu>();
        menuRef = menu.get();
        menuGroup_->addMenu(oid, std::move(menu));
    }
    // Retrieving menus and making sure the above were added.
    const auto* retrievedMenus = menuGroup_->menus();
    for (auto& [oid, menuRef] : menus) {
        EXPECT_EQ(retrievedMenus->at(oid).get(), menuRef);
    }
}

TEST_F(MenuGroupTest, MenuGroup_toProto) {
    // Adding two menus and setting expectations.
    std::vector<std::string> menus = {"menu1", "menu2"};
    for (auto& oid : menus) {
        std::unique_ptr<MockMenu> menu = std::make_unique<MockMenu>();
        EXPECT_CALL(*menu, toProto(::testing::_)).Times(1)
            .WillOnce(::testing::Invoke([&oid](catena::Menu& menu) {
                // Just enough to ensure its copying the proper menu.
                auto& name = *menu.mutable_name();
                name.mutable_display_strings()->insert({"en", oid});
            }));
        menuGroup_->addMenu(oid, std::move(menu));
    }
    // Calling toProto
    catena::MenuGroup protoMenuGroup;
    menuGroup_->toProto(protoMenuGroup, false);
    // Comparing names and menus
    for (auto& [lang, name] : name_) {
        EXPECT_EQ(protoMenuGroup.name().display_strings().at(lang), name);
    }
    for (auto& oid : menus) {
        EXPECT_EQ(protoMenuGroup.menus().at(oid).name().display_strings().at("en"), oid);
    }
}

TEST_F(MenuGroupTest, MenuGroup_toProtoShallow) {
    // Adding two menus and setting expectations.
    std::vector<std::string> menus = {"menu1", "menu2"};
    for (auto& oid : menus) {
        std::unique_ptr<MockMenu> menu = std::make_unique<MockMenu>();
        EXPECT_CALL(*menu, toProto(::testing::_)).Times(0);
        menuGroup_->addMenu(oid, std::move(menu));
    }
    // Calling toProto
    catena::MenuGroup protoMenuGroup;
    menuGroup_->toProto(protoMenuGroup, true);
    // Comparing names and menus
    for (auto& [lang, name] : name_) {
        EXPECT_EQ(protoMenuGroup.name().display_strings().at(lang), name);
    }
    EXPECT_TRUE(protoMenuGroup.menus().empty()) << "Menus should not be serialized in shallow mode";
}
