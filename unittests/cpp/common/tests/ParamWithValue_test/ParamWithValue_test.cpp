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
 * @author jason.chen@rossvideo.com
 * @date 25/10/08
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "Param_test.h"

#include "ParamDescriptor.h"
#include "Device.h"
 
using namespace catena::common;
using EmptyParam = ParamWithValue<EmptyValue>;

class ParamWithValueTest : public ParamTest<EmptyValue> {
    /*
     * Returns the value type of the parameter we are testing with.
     */
    st2138::ParamType type() const override { return st2138::ParamType::EMPTY; }
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
    EXPECT_CALL(authz_, readAuthz(testing::A<const IParamDescriptor&>())).Times(1).WillRepeatedly(testing::Return(true));
    static std::unordered_map<std::string, IParamDescriptor*> empty_sub_params;
    EXPECT_CALL(pd_, getAllSubParams()).WillRepeatedly(testing::ReturnRef(empty_sub_params));
    auto foundParam = param.getParam(path, authz_, rc_);
    // Checking results.
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::NOT_FOUND);
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
    st2138::Param outParam;
    rc_ = param.toProto(outParam, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(oid_, outParam.template_oid());
}
/*
 * TEST 8 - Testing <EMPTY>ParamWithValue.fromProto().
 */
TEST_F(ParamWithValueTest, FromProto) {
    EmptyParam param(emptyValue, pd_);
    st2138::Value protoValue;
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
    st2138::Value protoValue;
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
    EXPECT_CALL(pd_, type()).Times(1).WillOnce(testing::Return(st2138::ParamType::EMPTY));
    EXPECT_EQ(param.type().value(), st2138::ParamType::EMPTY);
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
        [](const st2138::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
            return nullptr;
        }););
    // param.executeCommand()
    st2138::Value testVal;
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    testVal.set_string_value("test");
    EXPECT_CALL(pd_, executeCommand(testing::_, testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([&testVal](const st2138::Value& value, const bool respond, catena::exception_with_status& rc, const IAuthorizer& authz) {
        EXPECT_EQ(value.string_value(), testVal.string_value());
        return nullptr;
    }));
    EXPECT_FALSE(param.executeCommand(testVal, true, rc, Authorizer::kAuthzDisabled));
    // param.addParam()
    std::string subOid = "sub_oid";
    MockParamDescriptor subPd;
    EXPECT_CALL(pd_, addSubParam(subOid, &subPd)).Times(1).WillOnce(testing::Return());
    EXPECT_NO_THROW(param.addParam(subOid, &subPd););
    // param.isArrayType()
    for (auto [type, expected] : std::vector<std::pair<st2138::ParamType, bool>>{
        {st2138::ParamType::UNDEFINED, false},
        {st2138::ParamType::EMPTY, false},
        {st2138::ParamType::INT32, false},
        {st2138::ParamType::FLOAT32, false},
        {st2138::ParamType::STRING, false},
        {st2138::ParamType::STRUCT, false},
        {st2138::ParamType::STRUCT_VARIANT, false},
        {st2138::ParamType::INT32_ARRAY, true},
        {st2138::ParamType::FLOAT32_ARRAY, true},
        {st2138::ParamType::STRING_ARRAY, true},
        {st2138::ParamType::BINARY, false},
        {st2138::ParamType::STRUCT_ARRAY, true},
        {st2138::ParamType::STRUCT_VARIANT_ARRAY, true},
        {st2138::ParamType::DATA, false}
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
    st2138::Param outParam;
    // pd_.toProto throws an error
    EXPECT_CALL(pd_, toProto(testing::An<st2138::Param&>(), testing::_)).WillOnce(testing::Throw(std::runtime_error("Test error")));
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
    st2138::ParamInfoResponse paramInfo;
    EXPECT_CALL(pd_, toProto(testing::An<st2138::ParamInfo&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](st2138::ParamInfo& p, const IAuthorizer&) {
            p.set_oid(oid_);
        }));
    rc_ = param.toProto(paramInfo, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(oid_, paramInfo.info().oid());
}
/*
 * TEST 13 - Testing paramWithValue.toProto(ParamInfo) error handling.
 * Two main error cases:
 * - pd_.toProto throws an error.
 * - Not authorized.
 */
TEST_F(ParamWithValueTest, ParamInfoToProto_Error) {
    EmptyParam param(emptyValue, pd_);
    st2138::ParamInfoResponse paramInfo;
    // pd_.toProto throws an error
    EXPECT_CALL(pd_, toProto(testing::An<st2138::ParamInfo&>(), testing::_)).WillOnce(testing::Throw(std::runtime_error("Test error")));
    EXPECT_THROW(param.toProto(paramInfo, authz_), std::runtime_error);
    // No read authz
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc_ = param.toProto(paramInfo, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "toProto should return PERMISSION_DENIED if Authorizer does not have readAuthz.";
}
 
/*
 * TEST 14 - Testing nested struct with explicitly initialized empty child fields.
 * Verifies that struct fields can be set to empty values and accessed.
 */
TEST_F(ParamWithValueTest, NestedParameterAccess) {
    // Create a parent parameter with a child parameter
    Device dm;
    catena::common::ParamDescriptor parentDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Audio Channel"}}, "", "", false, "f1", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
    catena::common::ParamWithValue<catena::common::EmptyValue> parentParam(catena::common::emptyValue, parentDescriptor, dm, false);

    catena::common::ParamDescriptor childParamDescriptor(st2138::ParamType::EMPTY, {}, {{"en", "EQ List"}}, "", "", false, "f2", "", nullptr, false, false, dm, 0, 0, 2, false, &parentDescriptor);
    catena::common::ParamWithValue<catena::common::EmptyValue> childParam(catena::common::emptyValue, childParamDescriptor, dm, false);
    
    // Expect two authorization checks (one for parent, one for child) - both should pass
    EXPECT_CALL(authz_, readAuthz(testing::A<const IParamDescriptor&>())).Times(2).WillRepeatedly(testing::Return(true));
    
    // Test: Try to get child parameter from parent
    Path path = Path("/f2");
    EXPECT_TRUE(parentParam.getParam(path, authz_, rc_));
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}
/*
 * TEST 15 Tests that getParam returns nullptr and sets INVALID_ARGUMENT status when called with an empty path.
 */
TEST_F(ParamWithValueTest, EmptyPathValidation) {
    // Create a parent parameter with a child parameter
    Device dm;
    catena::common::ParamDescriptor parentDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Audio Channel"}}, "", "", false, "f1", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
    catena::common::ParamWithValue<catena::common::EmptyValue> parentParam(catena::common::emptyValue, parentDescriptor, dm, false);

    // Expect one authorization check for the empty path scenario
    EXPECT_CALL(authz_, readAuthz(testing::A<const IParamDescriptor&>())).Times(1).WillRepeatedly(testing::Return(true));
    
    // Test: Empty path should be invalid and return error
    Path emptyPath = Path("");
    auto foundParam = parentParam.getParam(emptyPath, authz_, rc_);
    
    // When OID is empty, it should return nullptr and set INVALID_ARGUMENT status
    EXPECT_FALSE(foundParam) << "Empty OID should return nullptr";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 16 - Testing if getParam can correctly navigate a multi-level path.
 */
TEST_F(ParamWithValueTest, TwoLevelPathAccess) {
    // Create a parent parameter with a child parameter
    Device dm;
    
    catena::common::ParamDescriptor parentDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Audio Channel"}}, "", "", false, "f1", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
    catena::common::ParamWithValue<catena::common::EmptyValue> parentParam(catena::common::emptyValue, parentDescriptor, dm, false);

    catena::common::ParamDescriptor childParamDescriptor(st2138::ParamType::EMPTY, {}, {{"en", "EQ List"}}, "", "", false, "f2", "", nullptr, false, false, dm, 0, 0, 2, false, &parentDescriptor);
    catena::common::ParamWithValue<catena::common::EmptyValue> childParam(catena::common::emptyValue, childParamDescriptor, dm, false);
    
    catena::common::ParamDescriptor child2ParamDescriptor(st2138::ParamType::EMPTY, {}, {{"en", "EQ List2"}}, "", "", false, "f3", "", nullptr, false, false, dm, 0, 0, 2, false, &childParamDescriptor);
    catena::common::ParamWithValue<catena::common::EmptyValue> child2Param(catena::common::emptyValue, child2ParamDescriptor, dm, false);
    
    // Expect authorization checks for each level
    EXPECT_CALL(authz_, readAuthz(testing::A<const IParamDescriptor&>())).Times(3).WillRepeatedly(testing::Return(true));
    
    // Test: Navigate through multi-level path
    Path path = Path("/f2/f3");
    EXPECT_TRUE(parentParam.getParam(path, authz_, rc_));
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}
/*
 * TEST 17 - Testing getParam with numeric index on struct parameter.
 */
TEST_F(ParamWithValueTest, NumericIndexOnStructParameterFails) {
    // Create a parent parameter with a child parameter
    Device dm;
    
    catena::common::ParamDescriptor parentDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Audio Channel"}}, "", "", false, "f1", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
    catena::common::ParamWithValue<catena::common::EmptyValue> parentParam(catena::common::emptyValue, parentDescriptor, dm, false);

    // Create child with OID "0" so numeric index can find it
    catena::common::ParamDescriptor childParamDescriptor(st2138::ParamType::EMPTY, {}, {{"en", "Index 0"}}, "", "", false, "0", "", nullptr, false, false, dm, 0, 0, 2, false, &parentDescriptor);
    catena::common::ParamWithValue<catena::common::EmptyValue> childParam(catena::common::emptyValue, childParamDescriptor, dm, false);

    // Authorization should be checked for both parent and child
    EXPECT_CALL(authz_, readAuthz(testing::A<const IParamDescriptor&>())).Times(2).WillRepeatedly(testing::Return(true));
    
    // Test: Access struct parameter by numeric index "/0" - should now succeed
    Path path = Path("/0");
    EXPECT_TRUE(parentParam.getParam(path, authz_, rc_));
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

/*
 * TEST 18 - Testing getParam's authorization failure.
 * Verifies that getParam returns PERMISSION_DENIED when authorization check fails
 */
TEST_F(ParamWithValueTest, UnauthorizedParameterAccessDenied) {
    // Create a parent parameter with a child parameter
    Device dm;
    
    catena::common::ParamDescriptor parentDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Audio Channel"}}, "", "", false, "f1", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
    catena::common::ParamWithValue<catena::common::EmptyValue> parentParam(catena::common::emptyValue, parentDescriptor, dm, false);

    catena::common::ParamDescriptor childParamDescriptor(st2138::ParamType::EMPTY, {}, {{"en", "EQ List"}}, "", "", false, "f2", "", nullptr, false, false, dm, 0, 0, 2, false, &parentDescriptor);
    
    // Passes Authz once then fails the rest
    EXPECT_CALL(authz_, readAuthz(testing::A<const IParamDescriptor&>())).Times(2).WillOnce(testing::Return(true)).WillRepeatedly(testing::Return(false));
    
    // Test: Try to access child parameter without proper authorization
    Path path = Path("/f2");
    EXPECT_FALSE(parentParam.getParam(path, authz_, rc_));
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED);
}

// /*
//  * TEST 19 - Testing nested parameter access with numeric index (converted to string).
//  * Since EmptyValue allows both strings and indices, test index path that gets converted to string lookup.
//  */
// TEST_F(ParamWithValueTest, NestedParameterAccessWithIndex) {
//     Device dm;
//     catena::common::ParamDescriptor parentDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Audio Channel"}}, "", "", false, "f1", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
//     catena::common::ParamWithValue<catena::common::EmptyValue> parentParam(catena::common::emptyValue, parentDescriptor, dm, false);

//     // Create child with OID "0" to test index-to-string conversion
//     catena::common::ParamDescriptor childParamDescriptor(st2138::ParamType::EMPTY, {}, {{"en", "Index 0"}}, "", "", false, "0", "", nullptr, false, false, dm, 0, 0, 2, false, &parentDescriptor);
//     catena::common::ParamWithValue<catena::common::EmptyValue> childParam(catena::common::emptyValue, childParamDescriptor, dm, false);
    
//     EXPECT_CALL(authz_, readAuthz(testing::A<const IParamDescriptor&>())).Times(2).WillRepeatedly(testing::Return(true));
    
//     // Test: Access using index /0 which gets converted to string "0"
//     Path path = Path("/0");
//     EXPECT_TRUE(parentParam.getParam(path, authz_, rc_));
//     EXPECT_EQ(rc_.status, catena::StatusCode::OK);
// }


// /*
//  * TEST 21 - Testing multi-level path with mixed string and index segments.
//  */
// TEST_F(ParamWithValueTest, TwoLevelPathAccessWithIndex) {
//     Device dm;
    
//     catena::common::ParamDescriptor parentDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Audio Channel"}}, "", "", false, "f1", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
//     catena::common::ParamWithValue<catena::common::EmptyValue> parentParam(catena::common::emptyValue, parentDescriptor, dm, false);

//     catena::common::ParamDescriptor childParamDescriptor(st2138::ParamType::EMPTY, {}, {{"en", "EQ List"}}, "", "", false, "0", "", nullptr, false, false, dm, 0, 0, 2, false, &parentDescriptor);
//     catena::common::ParamWithValue<catena::common::EmptyValue> childParam(catena::common::emptyValue, childParamDescriptor, dm, false);
    
//     catena::common::ParamDescriptor child2ParamDescriptor(st2138::ParamType::EMPTY, {}, {{"en", "EQ List2"}}, "", "", false, "1", "", nullptr, false, false, dm, 0, 0, 2, false, &childParamDescriptor);
//     catena::common::ParamWithValue<catena::common::EmptyValue> child2Param(catena::common::emptyValue, child2ParamDescriptor, dm, false);
    
//     EXPECT_CALL(authz_, readAuthz(testing::A<const IParamDescriptor&>())).Times(3).WillRepeatedly(testing::Return(true));
    
//     // Test: Navigate through multi-level path using indices (converted to strings "0" and "1")
//     Path path = Path("/0/1");
//     EXPECT_TRUE(parentParam.getParam(path, authz_, rc_));
//     EXPECT_EQ(rc_.status, catena::StatusCode::OK);
// }

// /*
//  * TEST 22 - Testing authorization failure with index-based access.
//  */
// TEST_F(ParamWithValueTest, UnauthorizedParameterAccessDeniedWithIndex) {
//     Device dm;
    
//     catena::common::ParamDescriptor parentDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Audio Channel"}}, "", "", false, "f1", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
//     catena::common::ParamWithValue<catena::common::EmptyValue> parentParam(catena::common::emptyValue, parentDescriptor, dm, false);

//     catena::common::ParamDescriptor childParamDescriptor(st2138::ParamType::EMPTY, {}, {{"en", "EQ List"}}, "", "", false, "0", "", nullptr, false, false, dm, 0, 0, 2, false, &parentDescriptor);
//     catena::common::ParamWithValue<catena::common::EmptyValue> childParam(catena::common::emptyValue, childParamDescriptor, dm, false);
    
//     // Passes Authz once then fails the rest
//     EXPECT_CALL(authz_, readAuthz(testing::A<const IParamDescriptor&>())).Times(2).WillOnce(testing::Return(true)).WillRepeatedly(testing::Return(false));
    
//     // Test: Try to access child parameter by index without proper authorization
//     Path path = Path("/0");
//     EXPECT_FALSE(parentParam.getParam(path, authz_, rc_));
//     EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED);
// } 
