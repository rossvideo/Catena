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
 * @brief Mock implementation for the IParamDescriptor class.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/26
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

#include <gmock/gmock.h>
#include <IParamDescriptor.h>

namespace catena {
namespace common {

// Mock implementation for the IParamDescriptor class.
class MockParamDescriptor : public IParamDescriptor {
  public:
    MOCK_METHOD(ParamType, type, (), (const, override));
    MOCK_METHOD(const PolyglotText::DisplayStrings&, name, (), (const, override));
    MOCK_METHOD((const std::string&), getOid, (), (const, override));
    MOCK_METHOD(void, setOid, (const std::string& oid), (override));
    MOCK_METHOD(bool, hasTemplateOid, (), (const, override));
    MOCK_METHOD((const std::string&), templateOid, (), (const, override));
    MOCK_METHOD(bool, readOnly, (), (const, override));
    MOCK_METHOD(void, readOnly, (bool flag), (override));
    MOCK_METHOD((const std::string&), getScope, (), (const, override));
    MOCK_METHOD(bool, minimalSet, (), (const, override));
    MOCK_METHOD(void, setMinimalSet, (bool flag), (override));
    MOCK_METHOD(uint32_t, max_length, (), (const, override));
    MOCK_METHOD(std::size_t, total_length, (), (const, override));
    MOCK_METHOD(void, toProto, (catena::Param& param, Authorizer& authz), (const, override));
    MOCK_METHOD(void, toProto, (catena::ParamInfo& paramInfo, Authorizer& authz), (const, override));
    MOCK_METHOD(const std::string&, name, (const std::string& language), (const, override));
    MOCK_METHOD(void, addSubParam, (const std::string& oid, IParamDescriptor* item), (override));
    MOCK_METHOD(IParamDescriptor&, getSubParam, (const std::string& oid), (const, override));
    MOCK_METHOD((const std::unordered_map<std::string, IParamDescriptor*>&), getAllSubParams, (), (const, override));
    MOCK_METHOD(const catena::common::IConstraint*, getConstraint, (), (const, override));
    MOCK_METHOD(void, defineCommand, (std::function<std::unique_ptr<ICommandResponder>(catena::Value)> commandImpl), (override));
    MOCK_METHOD(std::unique_ptr<ICommandResponder>, executeCommand, (catena::Value value), (override));
    MOCK_METHOD(bool, isCommand, (), (const, override));
};

} // namespace common
} // namespace catena
