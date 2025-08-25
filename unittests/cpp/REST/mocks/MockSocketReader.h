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
 * @brief Mock implementation for the ISocketReader class.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/26
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

#include <gmock/gmock.h>
#include <interface/ISocketReader.h>

namespace catena {
namespace REST {

// Mock implementation for the ISocketReader class.
class MockSocketReader : public ISocketReader {
  public:
    MOCK_METHOD(void, read, (tcp::socket& socket), (override));
    MOCK_METHOD(RESTMethod, method, (), (const, override));
    MOCK_METHOD(const std::string&, endpoint, (), (const, override));
    MOCK_METHOD(uint32_t, slot, (), (const, override));
    MOCK_METHOD(const std::string&, fqoid, (), (const, override));
    MOCK_METHOD(bool, hasField, (const std::string& key), (const, override));
    MOCK_METHOD(const std::string&, fields, (const std::string& key), (const, override));
    MOCK_METHOD(const std::string&, jwsToken, (), (const, override));
    MOCK_METHOD(const std::string&, origin, (), (const, override));
    MOCK_METHOD(st2138::Device_DetailLevel, detailLevel, (), (const, override));
    MOCK_METHOD(const std::string&, jsonBody, (), (const, override));
    MOCK_METHOD(bool, stream, (), (const, override));
    MOCK_METHOD(IServiceImpl*, service, (), (override));
    MOCK_METHOD(IConnectionQueue&, connectionQueue, (), (override));
    MOCK_METHOD(bool, authorizationEnabled, (), (const, override));
    MOCK_METHOD(const std::string&, EOPath, (), (const, override));
    MOCK_METHOD(catena::common::ISubscriptionManager&, subscriptionManager, (), (override));

};

} // namespace REST
} // namespace catena
