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
 * @brief Mock implementation for the gRPC AuthContext class.
 * @author jason.chen@rossvideo.com
 * @date 25/11/18
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

#include <gmock/gmock.h>
#include <grpcpp/grpcpp.h>

namespace catena {
namespace gRPC {

// Mock implementation for the gRPC AuthContext class.
class MockAuthContext : public grpc::AuthContext {
  public:
    MOCK_METHOD(bool, IsPeerAuthenticated, (), (const, override));
    MOCK_METHOD(std::vector<grpc::string_ref>, GetPeerIdentity, (), (const, override));
    MOCK_METHOD(std::string, GetPeerIdentityPropertyName, (), (const, override));
    MOCK_METHOD(std::vector<grpc::string_ref>, FindPropertyValues, (const std::string& name), (const, override));
    MOCK_METHOD(grpc::AuthPropertyIterator, begin, (), (const, override));
    MOCK_METHOD(grpc::AuthPropertyIterator, end, (), (const, override));
    MOCK_METHOD(void, AddProperty, (const std::string& key, const grpc::string_ref& value), (override));
    MOCK_METHOD(bool, SetPeerIdentityPropertyName, (const std::string& name), (override));
};

} // namespace gRPC
} // namespace catena
