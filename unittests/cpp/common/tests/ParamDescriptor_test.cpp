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
 * @brief This file is for testing the ParamDescriptor.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/20
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "ParamDescriptor.h"
#include "PolyglotText.h"
#include "Enums.h"

#include "MockConstraint.h"
#include "MockDevice.h"
#include "MockParamDescriptor.h"

// gtest
#include <gtest/gtest.h>

using namespace catena::common;

class ParamDescriptorTest : public ::testing::Test {
  protected:
    void SetUp() override {
        create();
    }

    void create() {
        IParamDescriptor* parentPtr = hasParent ? &parent : nullptr;
        pd = std::make_unique<ParamDescriptor>(
            type, oidAliases, PolyglotText::ListInitializer{{"en", "name"}, {"fr", "nom"}},
            widget, scope, readOnly, oid, templateOid, &constraint, isCommand,
            dm, maxLength, totalLength, minimalSet, parentPtr
        );
    }

    std::unique_ptr<ParamDescriptor> pd = nullptr;

    catena::ParamType type = catena::ParamType::EMPTY;
    ParamDescriptor::OidAliases oidAliases = {"oid_alias1", "oid_alias2"};
    PolyglotText::DisplayStrings name = {{"en", "name"}, {"fr", "nom"}};
    std::string widget = "widget";
    std::string scope = "scope";
    bool readOnly = true;
    std::string oid = "oid";
    std::string templateOid = "template_oid";
    MockConstraint constraint;
    bool isCommand = true;
    MockDevice dm;
    uint32_t maxLength = 16;
    std::size_t totalLength = 16;
    bool minimalSet = true;
    bool hasParent = false;
    MockParamDescriptor parent;
};

TEST_F(ParamDescriptorTest, ParamDescriptor_Create) {
    // Test creation without parent.
    EXPECT_TRUE(pd) << "Failed to create ParamDescriptor without parent";
    // Test creation with parent.
    EXPECT_CALL(parent, addSubParam(oid, testing::_)).Times(1)
        .WillOnce(testing::Invoke([](const std::string& oid, IParamDescriptor* item){
            EXPECT_TRUE(item) << "Nullptr passed into addSubParam";
            return;
        }));
    hasParent = true;
    create();
    EXPECT_TRUE(pd) << "Failed to create ParamDescriptor with parent";
}

TEST_F(ParamDescriptorTest, ParamDescriptor_Getters) {
    EXPECT_EQ(type, pd->type());
    EXPECT_EQ(name, pd->name());
    EXPECT_EQ(name.at("en"), pd->name("en"));
    EXPECT_EQ(name.at("fr"), pd->name("fr"));
    EXPECT_EQ("", pd->name("unknown language")) << "Should return empty string for unknown language";
    EXPECT_EQ(oid, pd->getOid());
    EXPECT_EQ(!templateOid.empty(), pd->hasTemplateOid());
    EXPECT_EQ(templateOid, pd->templateOid());
    EXPECT_EQ(readOnly, pd->readOnly());
    EXPECT_EQ(minimalSet, pd->minimalSet());
    EXPECT_EQ(&constraint, pd->getConstraint());
    EXPECT_EQ(isCommand, pd->isCommand());
}

TEST_F(ParamDescriptorTest, ParamDescriptor_GetScope) {
    // Test scope with no parent
    EXPECT_EQ(scope, pd->getScope());
    // Test device scope
    std::string deviceScope = "device_scope";
    scope.clear();
    EXPECT_CALL(dm, getDefaultScope()).Times(1).WillOnce(testing::ReturnRef(deviceScope));
    create();
    EXPECT_EQ(deviceScope, pd->getScope()) << "Should return device's scope when scope is empty and there is no parent.";
    // Test scope with parent
    std::string parentScope = "parent_scope";
    hasParent = true;
    scope.clear(); // Clear scope to test parent's scope
    EXPECT_CALL(parent, addSubParam(oid, testing::_)).Times(1).WillOnce(testing::Return());
    EXPECT_CALL(parent, getScope()).Times(1).WillOnce(testing::ReturnRef(parentScope));
    create();
    EXPECT_EQ(parentScope, pd->getScope()) << "Should return parent's scope when scope is empty and there is a parent";
}

TEST_F(ParamDescriptorTest, ParamDescriptor_GetLengthConstraints) {
    EXPECT_EQ(maxLength, pd->max_length());
    EXPECT_EQ(totalLength, pd->total_length());
    // Reset to test default value
    maxLength = 0;
    totalLength = 0;
    uint32_t defaultMaxLength = 1024;
    uint32_t defaultTotalLength = 1024;
    EXPECT_CALL(dm, default_max_length()).Times(1).WillOnce(testing::Return(defaultMaxLength));
    EXPECT_CALL(dm, default_total_length()).Times(1).WillOnce(testing::Return(defaultTotalLength));
    create();
    EXPECT_EQ(defaultMaxLength, pd->max_length());
    EXPECT_EQ(defaultTotalLength, pd->total_length());
}

TEST_F(ParamDescriptorTest, ParamDescriptor_Setters) {
    // setOid(...)
    std::string newOid = "new_oid";
    pd->setOid(newOid);
    EXPECT_EQ(newOid, pd->getOid());
    // readOnly(...)
    pd->readOnly(!readOnly);
    EXPECT_EQ(!readOnly, pd->readOnly());
    // setMinimalSet(...)
    pd->setMinimalSet(!minimalSet);
    EXPECT_EQ(!minimalSet, pd->minimalSet());
}

TEST_F(ParamDescriptorTest, ParamDescriptor_SubParams) {
    MockParamDescriptor subPd1, subPd2;
    std::string subOid1 = "sub_oid1", subOid2 = "sub_oid2", subOid3 = "sub_oid3";
    // Adding sub parameters.
    pd->addSubParam(subOid1, &subPd1);
    pd->addSubParam(subOid2, &subPd2);
    EXPECT_THROW(pd->addSubParam(subOid3, nullptr), std::runtime_error) << "Should not add nullptr to sub params";
    // Getting sub parameters individually.
    EXPECT_EQ(&subPd1, &pd->getSubParam(subOid1));
    EXPECT_EQ(&subPd2, &pd->getSubParam(subOid2));
    EXPECT_THROW(pd->getSubParam(subOid3), std::runtime_error) << "Should throw error for non-existent sub param";
    // Getting all sub parameters.
    auto subParams = pd->getAllSubParams();
    EXPECT_TRUE(subParams.contains(subOid1));
    EXPECT_EQ(&subPd1, subParams[subOid1]);
    EXPECT_TRUE(subParams.contains(subOid2));
    EXPECT_EQ(&subPd2, subParams[subOid2]);
    EXPECT_FALSE(subParams.contains(subOid3)) << "Should not contain non-existent sub param";
}

TEST_F(ParamDescriptorTest, ParamDescriptor_ParamInfoToProto) {
    catena::ParamInfo paramInfo;
    pd->toProto(paramInfo, Authorizer::kAuthzDisabled);
    EXPECT_EQ(paramInfo.type(), type);
    EXPECT_EQ(paramInfo.oid(), oid);
    EXPECT_EQ(paramInfo.template_oid(), templateOid);
    EXPECT_EQ(paramInfo.name().display_strings_size(), name.size());
    for (const auto& [lang, str] : name) {
        EXPECT_EQ(paramInfo.name().display_strings().at(lang), str);
    }
}

TEST_F(ParamDescriptorTest, ParamDescriptor_ParamToProto) {
    // Adding sub parameters.
    MockParamDescriptor subPd1, subPd2;
    pd->addSubParam("sub_oid1", &subPd1);
    pd->addSubParam("sub_oid2", &subPd2);
    // Setting expectations.
    
    catena::Param param;
    std::string constraintOid = "constraint_oid";
    EXPECT_CALL(constraint, isShared()).Times(1).WillOnce(testing::Return(true));
    EXPECT_CALL(constraint, getOid()).Times(1).WillOnce(testing::ReturnRef(constraintOid));
    pd->toProto(param, Authorizer::kAuthzDisabled);
    EXPECT_EQ(param.type(), type);
    EXPECT_EQ(param.read_only(), readOnly);
    EXPECT_EQ(param.widget(), widget);
    EXPECT_EQ(param.minimal_set(), minimalSet);
    // Oid aliases
    EXPECT_EQ(param.oid_aliases_size(), readOnly);
    for (uint32_t i = 0; i < oidAliases.size(); ++i) {
        EXPECT_EQ(param.oid_aliases()[i], oidAliases[i]);
    }
    // Constraint
    EXPECT_EQ(param.constraint().ref_oid(), constraintOid);

}