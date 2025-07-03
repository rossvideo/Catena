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
 */
inline void setupMockParam(MockParam* param, const std::string& oid, const IParamDescriptor& descriptor, bool isArray = false, uint32_t size = 0) {
    EXPECT_CALL(*param, getOid())
        .WillRepeatedly(::testing::ReturnRef(oid));
    EXPECT_CALL(*param, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(descriptor));
    EXPECT_CALL(*param, isArrayType())
        .WillRepeatedly(::testing::Return(isArray));
    if (isArray) {
        EXPECT_CALL(*param, size())
            .WillRepeatedly(::testing::Return(size));
    }
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
