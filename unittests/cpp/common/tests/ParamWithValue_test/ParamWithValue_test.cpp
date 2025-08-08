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
 * @brief This file is for testing the ParamWithValue class.
 * Also test ParamWithEmptyValue.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/08/08
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "Param_test.h"

using namespace catena::common;
using EmptyParam = ParamWithValue<EmptyValue>;

class ParamWithValueTest : public ParamTest<EmptyValue> {
    /*
     * Returns the value type of the parameter we are testing with.
     */
    catena::ParamType type() const override { return catena::ParamType::EMPTY; }
};
/*
 * TEST 1 - Testing <EMPTY>ParamWithValue constructors.
 */
TEST_F(ParamWithValueTest, Create) {
    CreateTest(emptyValue);
}
/*
 * TEST 2 - Testing <EMPTY>ParamWithValue.get().
 */
TEST_F(ParamWithValueTest, Get) {
    GetValueTest(emptyValue);
}
/*
 * TEST 3 - Testing <EMPTY>ParamWithValue.size().
 */
TEST_F(ParamWithValueTest, Size) {
    EmptyParam param(emptyValue, pd_);
    EXPECT_EQ(param.size(), 0);
}
/*
 * TEST 4 - Testing <EMPTY>ParamWithValue.getParam().
 * EMPTY params have no sub-params and should return an error.
 */
TEST_F(ParamWithValueTest, GetParam) {
    EmptyParam param(emptyValue, pd_);
    Path path = Path("/test/oid");
    auto foundParam = param.getParam(path, authz_, rc_);
    // Checking results.
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 5 - Testing <EMPTY>ParamWithValue.addBack().
 * EMPTY params are not arrays, so this should return an error.
 */
TEST_F(ParamWithValueTest, AddBack) {
    EmptyParam param(emptyValue, pd_);
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to non-array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 6 - Testing <EMPTY>ParamWithValue.popBack().
 * EMPTY params are not arrays, so this should return an error.
 */
TEST_F(ParamWithValueTest, PopBack) {
    EmptyParam param(emptyValue, pd_);
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 7 - Testing <EMPTY>ParamWithValue.toProto().
 */
TEST_F(ParamWithValueTest, ParamToProto) {
    EmptyParam param(emptyValue, pd_);
    catena::Param outParam;
    rc_ = param.toProto(outParam, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(oid_, outParam.template_oid());
}
/*
 * TEST 8 - Testing <EMPTY>ParamWithValue.fromProto().
 */
TEST_F(ParamWithValueTest, FromProto) {
    EmptyParam param(emptyValue, pd_);
    catena::Value protoValue;
    protoValue.empty_value();
    rc_ = param.fromProto(protoValue, authz_);
    // Checking results.
    EXPECT_EQ(rc_.status,  catena::StatusCode::OK);
    EXPECT_EQ(&param.get(), &emptyValue);
}
/*
 * TEST 9 - Testing <INT>ParamWithValue.ValidateSetValue().
 */
TEST_F(ParamWithValueTest, ValidateSetValue) {
    EmptyParam param(emptyValue, pd_);
    catena::Value protoValue;
    protoValue.empty_value();
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_));
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 10 - Testing a number of functions that just forward to the descriptor.
 */
TEST_F(ParamWithValueTest, DescriptorForwards) {
    EmptyParam param(emptyValue, pd_);
    // param.getDescriptor()
    EXPECT_EQ(&param.getDescriptor(), &pd_);
    // param.type()
    EXPECT_CALL(pd_, type()).Times(1).WillOnce(testing::Return(catena::ParamType::EMPTY));
    EXPECT_EQ(param.type().value(), catena::ParamType::EMPTY);
    // param.getOid()
    EXPECT_CALL(pd_, getOid()).Times(1).WillOnce(testing::ReturnRef(oid_));
    EXPECT_EQ(param.getOid(), oid_);
    // param.setOid()
    std::string newOid = "new_oid";
    EXPECT_CALL(pd_, setOid(newOid)).Times(1).WillOnce(testing::Return());
    EXPECT_NO_THROW(param.setOid(newOid););
    // param.readOnly()
    EXPECT_CALL(pd_, readOnly()).Times(1).WillOnce(testing::Return(true));
    EXPECT_TRUE(param.readOnly());
    // param.readOnly(flag)
    EXPECT_CALL(pd_, readOnly(false)).Times(1).WillOnce(testing::Return());
    EXPECT_NO_THROW(param.readOnly(false););
    // param.defineCommand()
    EXPECT_CALL(pd_, defineCommand(testing::_)).Times(1).WillOnce(testing::Return());
    EXPECT_NO_THROW(param.defineCommand(
        [](const catena::Value& value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
            return nullptr;
        }););
    // param.executeCommand()
    catena::Value testVal;
    testVal.set_string_value("test");
    EXPECT_CALL(pd_, executeCommand(testing::_)).Times(1).WillOnce(testing::Invoke([&testVal](catena::Value value){
        EXPECT_EQ(value.string_value(), testVal.string_value());
        return nullptr;
    }));
    EXPECT_FALSE(param.executeCommand(testVal));
    // param.addParam()
    std::string subOid = "sub_oid";
    MockParamDescriptor subPd;
    EXPECT_CALL(pd_, addSubParam(subOid, &subPd)).Times(1).WillOnce(testing::Return());
    EXPECT_NO_THROW(param.addParam(subOid, &subPd););
    // param.isArrayType()
    for (auto [type, expected] : std::vector<std::pair<catena::ParamType, bool>>{
        {catena::ParamType::UNDEFINED, false},
        {catena::ParamType::EMPTY, false},
        {catena::ParamType::INT32, false},
        {catena::ParamType::FLOAT32, false},
        {catena::ParamType::STRING, false},
        {catena::ParamType::STRUCT, false},
        {catena::ParamType::STRUCT_VARIANT, false},
        {catena::ParamType::INT32_ARRAY, true},
        {catena::ParamType::FLOAT32_ARRAY, true},
        {catena::ParamType::STRING_ARRAY, true},
        {catena::ParamType::BINARY, false},
        {catena::ParamType::STRUCT_ARRAY, true},
        {catena::ParamType::STRUCT_VARIANT_ARRAY, true},
        {catena::ParamType::DATA, false}
    }) {
        MockParamDescriptor typeTestPd_;
        EmptyParam typeTestParam(emptyValue, typeTestPd_);
        EXPECT_CALL(typeTestPd_, type()).WillOnce(testing::Return(type));
        EXPECT_EQ(typeTestParam.isArrayType(), expected);
    }
    // param.getConstraint()
    MockConstraint testConstraint;
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&testConstraint));
    EXPECT_EQ(param.getConstraint(), &testConstraint);
    // param.getScope()
    std::string testScope = "test_scope";
    EXPECT_CALL(pd_, getScope()).Times(1).WillOnce(testing::ReturnRef(testScope));
    EXPECT_EQ(param.getScope(), testScope);
}
/*
 * TEST 11 - Testing paramWithValue.copy().
 */
TEST_F(ParamWithValueTest, Copy) {
    int32_t value{0};
    ParamWithValue<int32_t> param(value, pd_);
    std::unique_ptr<IParam> paramCopy = nullptr;
    // Copying param and checking its values.
    EXPECT_NO_THROW(paramCopy = param.copy()) << "Failed to copy ParamWithValue using copy()";
    ASSERT_TRUE(paramCopy) << "ParamWithValue copy is nullptr";
    EXPECT_EQ(&getParamValue<int32_t>(paramCopy.get()), &param.get());
    EXPECT_EQ(&paramCopy->getDescriptor(), &param.getDescriptor());
}
/*
 * TEST 11 - Testing paramWithValue.toProto(Param) error handling.
 * Two main error cases:
 * - pd_.toProto throws an error.
 * - Not authorized.
 */
TEST_F(ParamWithValueTest, ParamToProto_Error) {
    int32_t value{16};
    ParamWithValue<int32_t> param(value, pd_);
    catena::Param outParam;
    // pd_.toProto throws an error
    EXPECT_CALL(pd_, toProto(testing::An<catena::Param&>(), testing::_)).WillOnce(testing::Throw(std::runtime_error("Test error")));
    EXPECT_THROW(param.toProto(outParam, authz_), std::runtime_error);
    // Not authorized
    outParam.Clear();
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc_ = param.toProto(outParam, authz_);
    EXPECT_FALSE(outParam.value().has_int32_value())
        << "toProto should not set value if Authorizer does not have readAuthz.";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "toProto should return PERMISSION_DENIED if Authorizer does not have readAuthz.";
}
/*
 * TEST 12 - Testing paramWithValue.toProto(ParamInfo).
 */
TEST_F(ParamWithValueTest, ParamInfoToProto) {
    EmptyParam param(emptyValue, pd_);
    catena::ParamInfoResponse paramInfo;
    EXPECT_CALL(pd_, toProto(testing::An<catena::ParamInfo&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::ParamInfo& p, const IAuthorizer&) {
            p.set_oid(oid_);
        }));
    rc_ = param.toProto(paramInfo, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(oid_, paramInfo.info().oid());
}
/*
 * TEST 12 - Testing paramWithValue.toProto(ParamInfo) error handling.
 * Two main error cases:
 * - pd_.toProto throws an error.
 * - Not authorized.
 */
TEST_F(ParamWithValueTest, ParamInfoToProto_Error) {
    EmptyParam param(emptyValue, pd_);
    catena::ParamInfoResponse paramInfo;
    // pd_.toProto throws an error
    EXPECT_CALL(pd_, toProto(testing::An<catena::ParamInfo&>(), testing::_)).WillOnce(testing::Throw(std::runtime_error("Test error")));
    EXPECT_THROW(param.toProto(paramInfo, authz_), std::runtime_error);
    // No read authz
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc_ = param.toProto(paramInfo, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "toProto should return PERMISSION_DENIED if Authorizer does not have readAuthz.";
}
