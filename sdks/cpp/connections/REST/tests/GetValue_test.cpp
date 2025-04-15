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
 * @brief This file is for testing the GetValue.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/04/15
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

#include <Device.h>
#include "controllers/GetValue.h"
#include "interface/ISocketReader.h"

// Mocking the SocketReader interface
class MockSocketReader : public catena::REST::ISocketReader {
  public:
    MOCK_METHOD(void, read, (tcp::socket& socket, bool authz), (override));
    MOCK_METHOD(const std::string&, method, (), (const, override));
    MOCK_METHOD(const std::string&, rpc, (), (const, override));
    MOCK_METHOD(void, fields, ((std::unordered_map<std::string, std::string>&) fieldMap), (const, override));
    MOCK_METHOD(const std::string&, jwsToken, (), (const, override));
    MOCK_METHOD(const std::string&, jsonBody, (), (const, override));
    MOCK_METHOD(const std::string&, origin, (), (const, override));
    MOCK_METHOD(const std::string&, userAgent, (), (const, override));
    MOCK_METHOD(bool, authorizationEnabled, (), (const, override));

    void fields(std::unordered_map<std::string, std::string>& fieldMap) {
        fieldMap["oid" ] = "text_box";
        fieldMap["slot"] = "1";
    }
    const std::string& jwsToken() { return " "; }
    bool authorizationEnabled() { return false; }
};

// Mocking the Device interface
class MockDevice : public catena::common::Device {
  public:
    MOCK_METHOD(void, slot, (const uint32_t slot), ());
    MOCK_METHOD(uint32_t, slot, (), (const));
    MOCK_METHOD(void, detail_level, (const Device::DetailLevel_e detail_level), ());
    MOCK_METHOD(Device::DetailLevel_e, detail_level, (), (const));
    MOCK_METHOD(const std::string&, getDefaultScope, (), (const));
    MOCK_METHOD(bool, subscriptions, (), (const));
    MOCK_METHOD(uint32_t, default_max_length, (), (const));
    MOCK_METHOD(uint32_t, default_total_length, (), (const));
    MOCK_METHOD(void, set_default_max_length, (const uint32_t default_max_length), ());
    MOCK_METHOD(void, set_default_total_length, (const uint32_t default_total_length), ());
    MOCK_METHOD(void, toProto, (::catena::Device& dst, catena::common::Authorizer& authz, bool shallow), (const));
    MOCK_METHOD(void, toProto, (::catena::LanguagePacks& packs), (const));
    MOCK_METHOD(void, toProto, (::catena::LanguageList& list), (const));
    MOCK_METHOD(catena::exception_with_status, addLanguage, (catena::AddLanguagePayload& language, catena::common::Authorizer& authz), ());
    MOCK_METHOD(catena::exception_with_status, getLanguagePack, (const std::string& languageId, Device::ComponentLanguagePack& pack), (const));
    MOCK_METHOD(Device::DeviceSerializer, getComponentSerializer, (catena::common::Authorizer& authz, bool shallow), (const));
    MOCK_METHOD(Device::DeviceSerializer, getComponentSerializer, (catena::common::Authorizer& authz, const std::vector<std::string>& subscribed_oids, bool shallow), (const));
    MOCK_METHOD(bool, tryMultiSetValue, (catena::MultiSetValuePayload src, catena::exception_with_status& ans, catena::common::Authorizer& authz), ());
    MOCK_METHOD(catena::exception_with_status, commitMultiSetValue, (catena::MultiSetValuePayload src, catena::common::Authorizer& authz), ());
    MOCK_METHOD(catena::exception_with_status, setValue, (const std::string& jptr, catena::Value& src, catena::common::Authorizer& authz), ());
    MOCK_METHOD(catena::exception_with_status, getValue, (const std::string& jptr, catena::Value& value, catena::common::Authorizer& authz), (const));
    MOCK_METHOD(bool, shouldSendParam, (const catena::common::IParam& param, bool is_subscribed, catena::common::Authorizer& authz), (const));

    catena::exception_with_status getValue(const std::string& jptr, catena::Value& value, catena::common::Authorizer& authz) const {
        catena::exception_with_status rc("OK", catena::StatusCode::OK);
        if (jptr != "/text_box") {
            rc = catena::exception_with_status("Invalid OID", catena::StatusCode::INVALID_ARGUMENT);
        } else {
            auto status = google::protobuf::util::JsonStringToMessage(absl::string_view("{\n \"stringValue\": \"Hello, World!\"\n}"), &value);
        }
        return rc;
    }

};

// Fixture
class RESTGetValueTests : public ::testing::Test {
  protected:
    void SetUp() override {
        clientSocket = tcp::socket(io_context);
        serverSocket = tcp::socket(io_context);
        acceptor = tcp::acceptor(io_context, tcp::endpoint(tcp::v4(), 0));
        // Linking client and server sockets.
        clientSocket.connect(acceptor.local_endpoint());
        acceptor.accept(serverSocket);
    }

    void TearDown() override {
        // Cleanup code here
    }

    boost::asio::io_context io_context;
    tcp::socket clientSocket;
    tcp::socket serverSocket;
    tcp::acceptor acceptor;
    MockSocketReader context;
    MockDevice dm;
};

TEST_F(RESTGetValueTests, GetValue_constructor) {
    // Creating GetValue object.
    catena::REST::GetValue getValue(serverSocket, context, dm);
}
 