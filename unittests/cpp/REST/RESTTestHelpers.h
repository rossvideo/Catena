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
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief Helper functions for REST API tests
 * @author Zuhayr Sarker (zuhayr.sarker@rossvideo.com)
 * @date 2025-05-22
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <map>

// Forward declarations
namespace catena {
namespace common {
class MockParam;
} // namespace common
} // namespace catena

namespace catena {
namespace REST {
namespace test {

/*
 * ============================================================================
 *                        BasicParamInfoRequest Helpers
 * ============================================================================
 */

struct ParamInfo {
    std::string oid;
    // std::string name; // Might be irrelevant
    catena::ParamType type;
    // std::string template_oid = ""; // Might be irrelevant
    uint32_t array_length = 0;
    catena::StatusCode status = catena::StatusCode::OK;  // Default to OK
};

/**
 * @brief Creates or populates a BasicParamInfoResponse with the specified parameters
 */
inline void setupParamInfo(catena::BasicParamInfoResponse& response, const ParamInfo& info) {
    response.mutable_info()->set_oid(info.oid);
    // response.mutable_info()->mutable_name()->mutable_display_strings()->insert({"en", info.name}); // Might be irrelevant
    response.mutable_info()->set_type(info.type);
    // response.mutable_info()->set_template_oid(info.template_oid); // Might be irrelevant
    response.set_array_length(info.array_length);
}

/**
 * @brief Sets up a mock parameter to return a BasicParamInfoResponse
 *        Also sets up getDescriptor, isArrayType, and size for array types if descriptor is provided.
 */
inline void setupMockParam(catena::common::MockParam* mockParam, const ParamInfo& info, const catena::common::IParamDescriptor* descriptor = nullptr) {
    EXPECT_CALL(*mockParam, getOid())
        .WillRepeatedly(::testing::ReturnRef(info.oid));

    // Set up getDescriptor if provided
    if (descriptor) {
        EXPECT_CALL(*mockParam, getDescriptor())
            .WillRepeatedly(::testing::ReturnRef(*descriptor));
    }

    // Set up isArrayType and size if array_length > 0
    if (info.array_length > 0) {
        EXPECT_CALL(*mockParam, isArrayType())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockParam, size())
            .WillRepeatedly(::testing::Return(info.array_length));
    } else {
        EXPECT_CALL(*mockParam, isArrayType())
            .WillRepeatedly(::testing::Return(false));
    }

    // Only expect toProto if status indicates success (HTTP status < 300)
    if (catena::REST::codeMap_.at(info.status).first < 300) {
        EXPECT_CALL(*mockParam, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::_))
            .WillRepeatedly(::testing::Invoke([info](catena::BasicParamInfoResponse& response, catena::common::Authorizer&) {
                setupParamInfo(response, info);
                return catena::exception_with_status("", catena::StatusCode::OK);
            }));
    }
}

/**
 * @brief Creates and serializes a BasicParamInfoResponse to JSON
 * @return The serialized JSON string
 */
inline std::string createParamInfoJson(const ParamInfo& info) {
    catena::BasicParamInfoResponse response;
    setupParamInfo(response, info);
    std::string jsonBody;
    google::protobuf::util::JsonPrintOptions options;
    auto status = google::protobuf::util::MessageToJsonString(response, &jsonBody, options);
    return jsonBody;
}

/*
 * ============================================================================
 *                        DeviceRequest Helpers
 * ============================================================================
 */

/**
 * @brief Helper class to populate DeviceComponent objects with expected values
 * Similar to the gRPC test's StreamReader pattern
 */
class DeviceComponentHelper {
public:
    DeviceComponentHelper() {
        // Create expected values with one of everything, similar to gRPC test
        expVals.push_back(catena::DeviceComponent()); // [0] Device
        expVals.push_back(catena::DeviceComponent()); // [1] Menu
        expVals.push_back(catena::DeviceComponent()); // [2] Language pack
        expVals.push_back(catena::DeviceComponent()); // [3] Constraint
        expVals.push_back(catena::DeviceComponent()); // [4] Param
        expVals.push_back(catena::DeviceComponent()); // [5] Command
        
        expVals[0].mutable_device()->set_slot(1);
        expVals[1].mutable_menu()->set_oid("menu_test");
        expVals[2].mutable_language_pack()->set_language("language_test");
        expVals[3].mutable_shared_constraint()->set_oid("constraint_test");
        expVals[4].mutable_param()->set_oid("param_test");
        expVals[5].mutable_command()->set_oid("command_test");
    }
    
    /**
     * @brief Serializes a DeviceComponent to JSON string
     * @param component The DeviceComponent to serialize
     * @return The serialized JSON string
     */
    std::string serializeToJson(const catena::DeviceComponent& component) {
        std::string jsonString;
        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = false;
        auto status = google::protobuf::util::MessageToJsonString(component, &jsonString, options);
        if (!status.ok()) {
            return "{}"; // Return empty object if serialization fails
        }
        return jsonString;
    }
    
    /**
     * @brief Creates expected response JSON body from components
     * @param components Vector of DeviceComponent objects
     * @return The JSON body in the format {"data":[component1,component2,...]}
     */
    std::string createExpectedJsonBody(const std::vector<catena::DeviceComponent>& components) {
        std::string jsonBody = "{\"data\":[";
        for (size_t i = 0; i < components.size(); ++i) {
            if (i > 0) jsonBody += ",";
            jsonBody += serializeToJson(components[i]);
        }
        jsonBody += "]}";
        return jsonBody;
    }
    
    std::vector<catena::DeviceComponent> expVals;
};

} // namespace test
} // namespace REST
} // namespace catena 