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
#include "MockAuthorizer.h"

// gtest
#include <gtest/gtest.h>

using namespace catena::common;

class ParamDescriptorTest : public ::testing::Test {
  protected:
    /*
     * Initializes pd with default values used in most tests.
     */
    void SetUp() override {
        create();
        // authz_ by default has read and write authz for parent.
        EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(parent)))).WillRepeatedly(testing::Return(true));
        EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(parent)))).WillRepeatedly(testing::Return(true));
    }
    /*
     * Initializes pd using member values.
     */
    void create() {
        IConstraint* constraintPtr = hasConstraint ? &constraint : nullptr;
        IParamDescriptor* parentPtr = hasParent ? &parent : nullptr;
        pd = std::make_unique<ParamDescriptor>(
            type, oidAliases, PolyglotText::ListInitializer{{"en", "name"}, {"fr", "nom"}},
            widget, scope, readOnly, oid, templateOid, constraintPtr, isCommand, respond,
            dm, maxLength, totalLength, precision, minimalSet, parentPtr
        );
        // authz_ by default has read and write authz.
        EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(*pd)))).WillRepeatedly(testing::Return(true));
        EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(*pd)))).WillRepeatedly(testing::Return(true));
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
    bool hasConstraint = true; // If false, nullptr will be passed into pd as the constraint.
    MockConstraint constraint;
    bool isCommand = false;
    bool respond = true;
    MockDevice dm;
    uint32_t maxLength = 16;
    std::size_t totalLength = 16;
    uint32_t precision = 2;
    bool minimalSet = true;
    bool hasParent = false; // If false, nullptr will be passed into pd as the parent.
    MockParamDescriptor parent;
    MockAuthorizer authz_;
};

/*
 * TEST 1 - Testing ParamDescriptor Constructor with and without parent param.
 */
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
/*
 * TEST 2 - Testing ParamDescriptor Getters.
 */
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
    EXPECT_EQ(precision, pd->precision());
    EXPECT_EQ(minimalSet, pd->minimalSet());
    EXPECT_EQ(&constraint, pd->getConstraint());
    EXPECT_EQ(isCommand, pd->isCommand());
}
/*
 * TEST 3 - Testing ParamDescriptor GetScope().
 */
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
/*
 * TEST 4 - Testing ParamDescriptor max_length() and total_length().
 */
TEST_F(ParamDescriptorTest, ParamDescriptor_GetLengthConstraints) {
    EXPECT_EQ(maxLength, pd->max_length())     << "max_length should return the param's max_length value when >0";
    EXPECT_EQ(totalLength, pd->total_length()) << "total_length should return the param's total_length value when >0";
    // Resetting to test with default lengths from device.
    maxLength = 0;
    totalLength = 0;
    uint32_t defaultMaxLength = 1024;
    uint32_t defaultTotalLength = 1024;
    EXPECT_CALL(dm, default_max_length()).Times(1).WillOnce(testing::Return(defaultMaxLength));
    EXPECT_CALL(dm, default_total_length()).Times(1).WillOnce(testing::Return(defaultTotalLength));
    create();
    EXPECT_EQ(defaultMaxLength, pd->max_length())     << "max_length should return the device's max_length value when set to 0";
    EXPECT_EQ(defaultTotalLength, pd->total_length()) << "total_length should return the device's total_length value when set to 0";
}
/*
 * TEST 5 - Testing ParamDescriptor Setters.
 */
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
/*
 * TEST 6 - Testing ParamDescriptor SubParam functions.
 */
TEST_F(ParamDescriptorTest, ParamDescriptor_SubParams) {
    // Test params.
    MockParamDescriptor subPd1, subPd2;
    std::string subOid1 = "sub_oid1", subOid2 = "sub_oid2", subOid3 = "sub_oid3";
    // Adding sub parameters.
    pd->addSubParam(subOid1, &subPd1);
    pd->addSubParam(subOid2, &subPd2);
    EXPECT_THROW(pd->addSubParam(subOid3, nullptr), std::runtime_error) << "Should not add nullptr to sub params";
    // Getting sub parameters individually.
    EXPECT_EQ(&subPd1, &pd->getSubParam(subOid1)) << "Should return the added sub param \"subOid1\"";
    EXPECT_EQ(&subPd2, &pd->getSubParam(subOid2)) << "Should return the added sub param \"subOid2\"";
    EXPECT_THROW(pd->getSubParam(subOid3), std::runtime_error) << "Should throw error for non-existent sub param";
    // Getting all sub parameters.
    auto subParams = pd->getAllSubParams();
    EXPECT_TRUE(subParams.contains(subOid1))  << "SubParams should include added sub param \"subOid1\"";
    EXPECT_EQ(&subPd1, subParams[subOid1]);
    EXPECT_TRUE(subParams.contains(subOid2))  << "SubParams should include added sub param \"subOid2\"";
    EXPECT_EQ(&subPd2, subParams[subOid2]);
    EXPECT_FALSE(subParams.contains(subOid3)) << "Should not contain non-existent sub param";
}
/*
 * TEST 7 - Testing ParamDescriptor toProto with ParamInfo object.
 */
TEST_F(ParamDescriptorTest, ParamDescriptor_ParamInfoToProto) {
    catena::ParamInfo paramInfo;
    pd->toProto(paramInfo, authz_);
    EXPECT_EQ(paramInfo.type(), type);
    EXPECT_EQ(paramInfo.oid(), oid);
    EXPECT_EQ(paramInfo.template_oid(), templateOid);
    EXPECT_EQ(paramInfo.name().display_strings_size(), name.size());
    for (const auto& [lang, str] : name) {
        EXPECT_EQ(paramInfo.name().display_strings().at(lang), str);
    }
}
/*
 * TEST 8 - Testing ParamDescriptor toProto with Param object.
 */
TEST_F(ParamDescriptorTest, ParamDescriptor_ParamToProto) {
    // Adding sub parameters.
    MockParamDescriptor subPd1, subPd2;
    std::string subOid1 = "sub_oid1", subOid2 = "sub_oid2";
    pd->addSubParam(subOid1, &subPd1);
    pd->addSubParam(subOid2, &subPd2);
    // Setting expectations.
    std::string constraintOid = "constraint_oid";
    EXPECT_CALL(constraint, isShared()).Times(2)
        .WillOnce(testing::Return(true))   // Shared constraint on first test.
        .WillOnce(testing::Return(false)); // Unique constraint on second test.
    EXPECT_CALL(constraint, getOid()).Times(1).WillOnce(testing::ReturnRef(constraintOid));
    EXPECT_CALL(constraint, toProto(testing::_)).Times(1)
        .WillOnce(testing::Invoke([](catena::Constraint &constraint){
            constraint.set_ref_oid("constraint_oid");
        }));

    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(subPd1)))).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(subPd1, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([&subOid1](catena::Param& param, const IAuthorizer& authz) {
            param.add_oid_aliases(subOid1);
        }));
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(subPd2)))).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(subPd2, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([&subOid2](catena::Param& param, const IAuthorizer& authz) {
            param.add_oid_aliases(subOid2);
        }));
    // Calling toProto and checking the result.
    catena::Param param;
    pd->toProto(param, authz_);
    EXPECT_EQ(param.type(), type);
    EXPECT_EQ(param.read_only(), readOnly);
    EXPECT_EQ(param.widget(), widget);
    EXPECT_EQ(param.minimal_set(), minimalSet);
    // Oid aliases
    EXPECT_EQ(param.oid_aliases_size(), oidAliases.size());
    for (uint32_t i = 0; i < oidAliases.size(); ++i) {
        EXPECT_EQ(param.oid_aliases()[i], oidAliases[i]);
    }
    // Sub params
    EXPECT_EQ(param.params_size(), 2);
    EXPECT_EQ(param.params().at(subOid1).oid_aliases()[0], subOid1);
    EXPECT_EQ(param.params().at(subOid2).oid_aliases()[0], subOid2);
    // Shared constraint
    EXPECT_EQ(param.constraint().ref_oid(), constraintOid) << "Shared constraint should set ref_oid";
    // Resetting and testing with unique constraint.
    param.Clear();
    create();
    pd->toProto(param, authz_);
    EXPECT_EQ(param.constraint().ref_oid(), constraintOid) << "Unique constraint toProto should set ref_oid";
    // Resetting and testing with no constraint
    param.Clear();
    hasConstraint = false;
    create();
    pd->toProto(param, authz_);
    EXPECT_FALSE(param.has_constraint()) << "Param should not have a constraint";
}
/*
 * TEST 9 - Testing ParamDescriptor ExecuteCommand default command definition.
 */
TEST_F(ParamDescriptorTest, ParamDescriptor_ExecuteCommand) {
    catena::Value input;
    bool respond = false;
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto responder = pd->executeCommand(input, respond, status, authz_);
    auto response = responder->getNext();
    EXPECT_TRUE(response.has_exception()) << "Default command definition should return an \"UNIMPLEMENTED\" exception.";
    EXPECT_EQ(response.exception().type(), "UNIMPLEMENTED");
}
/*
 * TEST 10 - Testing ParamDescriptor DefineCommand.
 */
TEST_F(ParamDescriptorTest, ParamDescriptor_DefineCommand) {    
    {
    // Calling defineCommand with non-command parameter.
    EXPECT_THROW(pd->defineCommand([](const catena::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            catena::CommandResponse response;
            co_return response;
        }(value, respond));
    });, std::runtime_error) << "defineCommand() should throw an error if the param isCommand == False";
    // Testing response
    catena::Value input;
    bool respond = true;
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto responder = pd->executeCommand(input, respond, status, authz_);
    auto response = responder->getNext();
    EXPECT_TRUE(response.has_exception()) << "Non-command param should retain the default command definition.";
    EXPECT_EQ(response.exception().type(), "UNIMPLEMENTED");
    }
    {
    // Creating command ParamDescriptor.
    isCommand = true;
    create();
    // Defining command and executing.
    pd->defineCommand([](const catena::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            EXPECT_EQ(value.string_value(), "Test input") << "Input value not passed correctly to command.";
            catena::CommandResponse response;
            // Response #1
            response.mutable_response()->set_string_value("Command response 1");
            co_yield response;
            // Response #2
            response.Clear();
            response.mutable_response()->set_string_value("Command response 2");
            co_return response;
        }(value, respond));
    });
    catena::Value input;
    input.set_string_value("Test input");
    bool respond = true;
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto responder = pd->executeCommand(input, respond, status, authz_);
    // Testing response.
    catena::CommandResponse response;
    for (auto returnVal : {"Command response 1", "Command response 2"}) {
        EXPECT_TRUE(responder->hasMore())    << "Responder should have 2 responses.";
        response = responder->getNext();
        EXPECT_TRUE(response.has_response()) << "After a valid call to getNext() responder should have a response.";
        EXPECT_EQ(response.response().string_value(), returnVal);
    }
    EXPECT_FALSE(responder->hasMore())    << "Calls to hasMore() after all responses should return false.";
    response = responder->getNext();
    EXPECT_FALSE(response.has_response()) << "Calls to getNext() after all responses should not return a response.";
    }
}
/*
 * TEST 10 - Testing ParamDescriptor CommandResponder when client does not have write authz.
 */
TEST_F(ParamDescriptorTest, ParamDescriptor_CommandErrNoWriteAuthz) {    
    // Creating command ParamDescriptor.
    isCommand = true;
    create();
    catena::Value input;
    bool respond = true;
    catena::exception_with_status status{"", catena::StatusCode::OK};
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(*pd)))).Times(1).WillOnce(testing::Return(false));
    auto responder = pd->executeCommand(input, respond, status, authz_);
    // Testing response.
    EXPECT_FALSE(responder) << "Responder should be nullptr when client does not have write authz.";
    EXPECT_EQ(status.status, catena::StatusCode::PERMISSION_DENIED) << "Status should be PERMISSION_DENIED when client does not have write authz.";
}
/*
 * TEST 11 - Testing ParamDescriptor CommandResponded when an error is thrown.
 */
TEST_F(ParamDescriptorTest, ParamDescriptor_CommandErrUnhandled) {    
    // Creating command ParamDescriptor.
    isCommand = true;
    create();
    // Defining command that throws an error and executing.
    pd->defineCommand([](const catena::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            throw std::runtime_error("Test error");

            catena::CommandResponse response;
            response.mutable_response()->set_string_value("Error should be thrown before recieving a response.");
            co_return response;
        }(value, respond));
    });
    catena::Value input;
    bool respond = true;
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto responder = pd->executeCommand(input, respond, status, authz_);
    // Testing response.
    catena::CommandResponse response;
    EXPECT_TRUE(responder->hasMore())                                 << "Responder should have at least 1 response.";
    EXPECT_THROW(response = responder->getNext(), std::runtime_error) << "Responder should rethrow error when command execution fails";
    EXPECT_FALSE(responder->hasMore())                                << "Responder should not have any more responses after an error.";
    EXPECT_FALSE(response.has_response())                             << "Response should not have a response after an error.";
}
