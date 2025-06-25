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
 * @brief This file is for testing the LanguagePack.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/25
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MockDevice.h"

#include "LanguagePack.h"

using namespace catena::common;


// Fixture
class LanguagePackTest : public ::testing::Test {
  protected:
    // Initializes testPack
    void SetUp() override {
        // Expect a call to dm_.addItem() when initializing testPack_.
        EXPECT_CALL(dm_, addItem(languageCode_, testing::An<ILanguagePack*>())).Times(1).WillOnce(
            testing::Invoke([](const std::string &key, ILanguagePack *item){
                EXPECT_TRUE(item) << "No item passed into dm.addItem()";
            }));
        testPack_.reset(new LanguagePack(languageCode_, name_, {words_[0], words_[1]}, dm_));
    }

    std::unique_ptr<LanguagePack> testPack_ = nullptr;
    std::string languageCode_ = "en";
    std::string name_ = "English";
    std::vector<std::pair<std::string, std::string>> words_ = {
        { "greeting", "Hello" },
        { "parting", "Goodbye" }
    };
    MockDevice dm_;
};

/*
 * TEST 1 - Testing creation of LanguagePack
 */
TEST_F(LanguagePackTest, LanguagePack_Create) {
    EXPECT_TRUE(testPack_) << "Failed to create LanguagePack";
}

/*
 * TEST 2 - Testing use of LanguagePack.begin() and LanguagePack.end()
 */
TEST_F(LanguagePackTest, LanguagePack_Iterator) {
    std::unordered_map<std::string, std::string> words;
    words.insert(testPack_->begin(), testPack_->end());
    for (auto& [key, value] : words_) {
        EXPECT_EQ(value, words.at(key));
    }
}

/*
 * TEST 3 - Testing use of LanguagePack.toProto()
 */
TEST_F(LanguagePackTest, LanguagePack_ToProto) {
    catena::LanguagePack protoPack;
    // Call toProto.
    testPack_->toProto(protoPack);
    // Compare results.
    EXPECT_EQ(protoPack.name(), name_);
    auto words = protoPack.words();
    for (auto& [key, value] : protoPack.words()) {
        EXPECT_EQ(value, words.at(key));
    }
}

/*
 * TEST 4 - Testing use of LanguagePack.fromProto()
 */
TEST_F(LanguagePackTest, LanguagePack_FromProto) {
    // Initi languagePack with french.
    catena::LanguagePack frenchPack;
    frenchPack.set_name("French");
    frenchPack.mutable_words()->insert({"greeting", "Bonjour"});
    frenchPack.mutable_words()->insert({"parting", "Au revoir"});
    // Call fromProto and get results using toProto.
    testPack_->fromProto(frenchPack);
    catena::LanguagePack protoPack;
    testPack_->toProto(protoPack);
    // Compare results.
    EXPECT_EQ(frenchPack.name(), protoPack.name());
    auto frenchWords = frenchPack.words();
    auto protoWords = protoPack.words();
    for (auto& [key, value] : frenchWords) {
        EXPECT_EQ(value, protoWords.at(key));
    }
}
