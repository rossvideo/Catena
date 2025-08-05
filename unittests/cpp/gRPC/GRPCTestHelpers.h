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
 * @brief Helper functions for gRPC tests
 * @author Zuhayr Sarker (zuhayr.sarker@rossvideo.com)
 * @date 2025-07-24
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

#include <interface/device.pb.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>

#include "MockParamDescriptor.h"
#include "MockParam.h"

namespace catena {
namespace gRPC {
namespace test {

/*
 * ============================================================================
 *                        ParamInfoRequest Helpers
 * ============================================================================
 */

struct ParamInfo {
    std::string oid;
    catena::ParamType type;
    uint32_t array_length = 0;
    catena::StatusCode status = catena::StatusCode::OK;  // Default to OK
};

/**
 * @brief Creates or populates a ParamInfoResponse with the specified parameters
 */
inline void setupParamInfo(catena::ParamInfoResponse& response, const ParamInfo& info) {
    response.mutable_info()->set_oid(info.oid);
    response.mutable_info()->set_type(info.type);
    if (info.array_length > 0) {
        response.set_array_length(info.array_length);
    }
}

/**
 * @brief Sets up a mock parameter to return a ParamInfoResponse
 *        Also sets up getDescriptor, isArrayType, and size for array types.
 */
inline void setupMockParamInfo(catena::common::MockParam& mockParam, const ParamInfo& info, const catena::common::MockParamDescriptor& descriptor) {
    EXPECT_CALL(mockParam, getOid())
        .WillRepeatedly(::testing::ReturnRef(info.oid));

    // Set up getDescriptor
    EXPECT_CALL(mockParam, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(descriptor));

    // Set up getScope for authorization checks (default to monitor scope)
    static const std::string scope = catena::common::Scopes().getForwardMap().at(catena::common::Scopes_e::kMonitor);
    EXPECT_CALL(mockParam, getScope())
        .WillRepeatedly(::testing::ReturnRef(scope));

    // Set up isArrayType and size if array_length > 0
    if (info.array_length > 0) {
        EXPECT_CALL(mockParam, isArrayType())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(mockParam, size())
            .WillRepeatedly(::testing::Return(info.array_length));
    } else {
        EXPECT_CALL(mockParam, isArrayType())
            .WillRepeatedly(::testing::Return(false));
    }

    // Set up toProto for successful status
    if (info.status == catena::StatusCode::OK) {
        EXPECT_CALL(mockParam, toProto(::testing::An<catena::ParamInfoResponse&>(), ::testing::_))
            .WillRepeatedly(::testing::Invoke([info](catena::ParamInfoResponse& response, const IAuthorizer&) {
                setupParamInfo(response, info);
                return catena::exception_with_status("", catena::StatusCode::OK);
            }));
    }
}

/**
 * @brief Creates a ParamInfoResponse with the specified parameters
 * @return The ParamInfoResponse object
 */
inline catena::ParamInfoResponse createParamInfoResponse(const ParamInfo& info) {
    catena::ParamInfoResponse response;
    setupParamInfo(response, info);
    return response;
}

/**
 * @brief Creates a vector of ParamInfoResponse objects from a vector of ParamInfo structs
 * @return The vector of ParamInfoResponse objects
 */
inline std::vector<catena::ParamInfoResponse> createParamInfoResponses(const std::vector<ParamInfo>& infos) {
    std::vector<catena::ParamInfoResponse> responses;
    responses.reserve(infos.size());
    
    for (const auto& info : infos) {
        responses.emplace_back(createParamInfoResponse(info));
    }
    
    return responses;
}

} // namespace test
} // namespace gRPC
} // namespace catena 