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
 * @brief This file is for testing the PicklistConstraint.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/07/02
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MockDevice.h"

#include "PicklistConstraint.h"

using namespace catena::common;


TEST(PicklistConstraintTest, PicklistConstraint_Create) {
    // Variables to reuse
    PicklistConstraint::ListInitializer choices = {"Choice1", "Choice2", "Choice3"};
    bool strict = true;
    std::string oid = "test_oid";
    bool shared = false;
    MockDevice dm;
    // Constructor with no device
    PicklistConstraint constraint(choices, strict, oid, shared);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_FALSE(constraint.isRange()) << "PicklistConstraint should not be a range constraint";
    // Constructor with device.
    EXPECT_CALL(dm, addItem(oid, testing::An<IConstraint*>())).Times(1).WillOnce(
        testing::Invoke([](const std::string &key, IConstraint *item){
            EXPECT_TRUE(item) << "No item passed into dm.addItem()";
        }));
    constraint = PicklistConstraint(choices, strict, oid, shared, dm);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_FALSE(constraint.isRange()) << "PicklistConstraint should not be a range constraint";
}

TEST(PicklistConstraintTest, PicklistConstraint_SatisfiedNotStrict) {
    PicklistConstraint constraint({"Choice1", "Choice2", "Choice3"}, false, "test_oid", false);
    catena::Value src;
    src.set_string_value("Choice4");
    EXPECT_TRUE(constraint.satisfied(src)) << "PicklistConstraint should be satisfied when not strict";
}

TEST(PicklistConstraintTest, PicklistConstraint_SatisfiedStrict) {
    
}

TEST(PicklistConstraintTest, PicklistConstraint_Apply) {
    
}

TEST(PicklistConstraintTest, PicklistConstraint_ToProto) {
    
}

TEST(PicklistConstraintTest, PicklistConstraint_ToProtoEmpty) {
    
}
