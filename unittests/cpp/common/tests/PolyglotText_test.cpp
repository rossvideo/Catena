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
 * @brief This file is for testing the PolyglotText.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/07/02
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "PolyglotText.h"

using namespace catena::common;

/*
 * TEST 1 - Testing PolyglotText constructors.
 */
TEST(PolyglotTextTest, PolyglotText_Create) {
    // Default constructor
    PolyglotText pt;
    EXPECT_TRUE(pt.displayStrings().empty()) << "Failed to create PolyglotText with default constructor";
    // Constructor with DisplayStrings
    PolyglotText::DisplayStrings displayStrings = {{"en", "Name"}, {"fr", "Nom"}};
    pt = PolyglotText(displayStrings);
    EXPECT_EQ(displayStrings, pt.displayStrings()) << "Failed to create PolyglotText with DisplayStrings constructor";
    // Constructor with initializer list
    displayStrings = {{"de", "name in german"}};
    pt = PolyglotText({{"de", "name in german"}});
    EXPECT_EQ(displayStrings, pt.displayStrings()) << "Failed to create PolyglotText with initializer list constructor";
}
/*
 * TEST 2 - Testing PolyglotText toProto.
 */
TEST(PolyglotTextTest, PolyglotText_Move) {
    // Creating PolyglotText;
    PolyglotText::DisplayStrings displayStrings = {{"en", "Name"}, {"fr", "Nom"}};
    PolyglotText pt(displayStrings);
    PolyglotText pt2(std::move(pt));
    EXPECT_EQ(displayStrings, pt2.displayStrings()) << "Failed to move PolyglotText";
}
/*
 * TEST 3 - Testing PolyglotText toProto.
 */
TEST(PolyglotTextTest, PolyglotText_ToProto) {
    // Creating PolyglotText;
    PolyglotText::DisplayStrings displayStrings = {{"en", "Name"}, {"fr", "Nom"}};
    PolyglotText pt(displayStrings);
    st2138::PolyglotText dst;
    pt.toProto(dst);
    EXPECT_EQ(PolyglotText::DisplayStrings(dst.display_strings().begin(), dst.display_strings().end()), displayStrings);
}
/*
 * TEST 4 - Testing PolyglotText toProto with no displayStrings.
 */
TEST(PolyglotTextTest, PolyglotText_ToProtoEmpty) {
    // Creating PolyglotText;
    PolyglotText pt;
    st2138::PolyglotText dst;
    pt.toProto(dst);
    EXPECT_TRUE(dst.display_strings().empty()) << "Protobuf object should be empty";
}
