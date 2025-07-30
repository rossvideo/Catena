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
 * @brief A collection of helper classes used across the tests.
 * @author zuhayr.sarker@rossvideo.com
 * @date 25/05/16
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

#include <gmock/gmock.h>
#include "MockParam.h"
#include "MockParamDescriptor.h"
#include "MockDevice.h"

namespace catena {
namespace common {

/**
 * @brief Helper function to set up common mock parameter expectations
 * @param param The mock parameter to set up
 * @param oid The OID to return
 * @param descriptor The descriptor to return
 * @param isArray Whether this is an array type (defaults to false)
 * @param size The array size if it's an array type (defaults to 0)
 * @param scope The scope to return (defaults to monitor)
 */
inline void setupMockParam(MockParam& param, const std::string& oid, MockParamDescriptor& descriptor, bool isArray = false, uint32_t size = 0, const std::string& scope = Scopes().getForwardMap().at(Scopes_e::kMonitor)) {
    EXPECT_CALL(param, getOid())
        .WillRepeatedly(::testing::ReturnRef(oid));
    EXPECT_CALL(param, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(descriptor));
    EXPECT_CALL(param, isArrayType())
        .WillRepeatedly(::testing::Return(isArray));
    EXPECT_CALL(param, getScope())
        .WillRepeatedly(::testing::ReturnRef(scope));
    if (isArray) {
        EXPECT_CALL(param, size())
            .WillRepeatedly(::testing::Return(size));
    }
    // Set default isCommand for params to be false
    EXPECT_CALL(descriptor, isCommand())
        .WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(descriptor, getScope())
        .WillRepeatedly(testing::ReturnRef(scope));

}

/**
 * @brief Helper function to get a JWS token for a specific scope
 * @param scope The scope string to get the token for (e.g., "st2138:mon", "st2138:op:w")
 * @return The JWS token for the specified scope, or empty string if not found
 */
inline std::string getJwsToken(const std::string& scope) {
    static const std::unordered_map<std::string, std::string> testTokens = {
        {Scopes().getForwardMap().at(Scopes_e::kMonitor),        
            "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uIiwiaWF0IjoxNTE2MjM5MDIyfQ.YkqS7hCxstpXulFnR98q0m088pUj6Cnf5vW6xPX8aBQ"},
        {Scopes().getForwardMap().at(Scopes_e::kOperate),        
            "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6b3AiLCJpYXQiOjE1MTYyMzkwMjJ9.lduNvr6tEaLFeIYR4bH5tC55WUSDBEe5PFz9rvGRD3o"},    
        {Scopes().getForwardMap().at(Scopes_e::kConfig),         
            "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6Y2ZnIiwiaWF0IjoxNTE2MjM5MDIyfQ.n1dZJ01l8z4urxFUsSbUoaSJgflK828BHSLcxqTxOf4"},
        {Scopes().getForwardMap().at(Scopes_e::kAdmin),          
            "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6YWRtIiwiaWF0IjoxNTE2MjM5MDIyfQ.nqkypNl8hTMWC8zF1aIA_CvsfoOdbZrYpr9JN4T4sDs"},
        {Scopes().getForwardMap().at(Scopes_e::kMonitor) + ":w", 
            "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOnciLCJpYXQiOjE1MTYyMzkwMjJ9.QTHN7uqmk_jR2nVumyee3gMki-47tKOm_R0jnhT8Tpk"},
        {Scopes().getForwardMap().at(Scopes_e::kOperate) + ":w", 
            "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6b3A6dyIsImlhdCI6MTUxNjIzOTAyMn0.SNndYRi4apWLZfp-BoosQtmDDNFInVcMCMuh7djz-QI"},
        {Scopes().getForwardMap().at(Scopes_e::kConfig)  + ":w", 
            "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6Y2ZnOnciLCJpYXQiOjE1MTYyMzkwMjJ9.ty50rEHLJUlseD_6bj7KrmCm9NXVwHjbTAv1u392HCs"},     
        {Scopes().getForwardMap().at(Scopes_e::kAdmin)   + ":w", 
            "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6YWRtOnciLCJpYXQiOjE1MTYyMzkwMjJ9.WrWmmNhw3EZ6AzZAytgZbvb_9NFL3_YtSSsZibW1P0w"},
        // No scope.
        {"", 
            "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c"},
        // Monitor and Operate Write Scope.
        {"st2138:mon st2138:op:w", 
            "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uIHN0MjEzODpvcDp3IiwiaWF0IjoxNTE2MjM5MDIyfQ.Z8upjHhZWKBlZ-yUcu7FFlJPby_C4jB9Bnk-DGxoQyM"},
        // All scopes with write permissions.
        {"st2138:mon:w st2138:op:w st2138:cfg:w st2138:adm:w", 
            "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOncgc3QyMTM4Om9wOncgc3QyMTM4OmNmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MTUxNjIzOTAyMn0.YkqS7hCxstpXulFnR98q0m088pUj6Cnf5vW6xPX8aBQ"}
    };
    
    auto it = testTokens.find(scope);
    return (it != testTokens.end()) ? it->second : "";
}

// Helper class for setting up parameter hierarchies in tests
class ParamHierarchyBuilder {
public:
    struct DescriptorInfo {
        std::shared_ptr<MockParamDescriptor> descriptor;
        std::shared_ptr<std::unordered_map<std::string, IParamDescriptor*>> subParams;
        std::string oid;
        DescriptorInfo() : subParams(std::make_shared<std::unordered_map<std::string, IParamDescriptor*>>()) {}
    };

    /**
     * @brief Creates a descriptor with the given OID and sets up its mock behavior
     * @param oid The OID to return
     * @return A DescriptorInfo struct containing the descriptor and its sub-parameters
     */
    static DescriptorInfo createDescriptor(const std::string& oid) {
        DescriptorInfo info;
        info.oid = oid; 
        info.descriptor = std::make_shared<MockParamDescriptor>();
        EXPECT_CALL(*info.descriptor, getOid())
            .WillRepeatedly(::testing::ReturnRef(info.oid)); 
        EXPECT_CALL(*info.descriptor, getAllSubParams())
            .WillRepeatedly(::testing::ReturnRef(*info.subParams));
        return info;
    }

    /**
     * @brief Adds a child parameter to a parent descriptor
     * @param parent The parent descriptor
     * @param name The name of the child parameter
     * @param child The child descriptor
     */
    static void addChild(DescriptorInfo& parent, const std::string& name, DescriptorInfo& child) {
        parent.subParams->emplace(name, child.descriptor.get());
    }
};

} // namespace common
} // namespace catena 
