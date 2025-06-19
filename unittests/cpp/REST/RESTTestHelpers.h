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
 *                        ParamInfoRequest Helpers
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
 * @brief Creates or populates a ParamInfoResponse with the specified parameters
 */
inline void setupParamInfo(catena::ParamInfoResponse& response, const ParamInfo& info) {
    response.mutable_info()->set_oid(info.oid);
    // response.mutable_info()->mutable_name()->mutable_display_strings()->insert({"en", info.name}); // Might be irrelevant
    response.mutable_info()->set_type(info.type);
    // response.mutable_info()->set_template_oid(info.template_oid); // Might be irrelevant
    response.set_array_length(info.array_length);
}

/**
 * @brief Sets up a mock parameter to return a ParamInfoResponse
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
        EXPECT_CALL(*mockParam, toProto(::testing::An<catena::ParamInfoResponse&>(), ::testing::_))
            .WillRepeatedly(::testing::Invoke([info](catena::ParamInfoResponse& response, catena::common::Authorizer&) {
                setupParamInfo(response, info);
                return catena::exception_with_status("", catena::StatusCode::OK);
            }));
    }
}

/**
 * @brief Creates and serializes a ParamInfoResponse to JSON
 * @return The serialized JSON string
 */
inline std::string createParamInfoJson(const ParamInfo& info) {
    catena::ParamInfoResponse response;
    setupParamInfo(response, info);
    std::string jsonBody;
    google::protobuf::util::JsonPrintOptions options;
    auto status = google::protobuf::util::MessageToJsonString(response, &jsonBody, options);
    return jsonBody;
}

} // namespace test
} // namespace REST
} // namespace catena 