/*
 * Copyright 2024 Ross Video Ltd
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

#include <ParamVisitor.h>
#include <exception>

namespace catena {
namespace common {

void traverseParams(IParam* param, const std::string& path, Device& device, ParamVisitor& visitor) {
    if (!param) return;

    // Visit the current parameter
    visitor.visit(param, path);

    // Handle array types specially
    if (param->isArrayType()) {
        uint32_t array_length = param->size();
        if (array_length > 0) {
            visitor.visitArray(param, path, array_length);
            
            // Process each array element
            for (uint32_t i = 0; i < array_length; i++) {
                Path indexed_path{path, std::to_string(i)};
                catena::exception_with_status rc{"", catena::StatusCode::OK};
                auto indexed_param = device.getParam(indexed_path.toString(), rc);
                if (indexed_param) {
                    visitor.visitArrayElement(indexed_param.get(), indexed_path.toString(), i);
                    traverseParams(indexed_param.get(), indexed_path.toString(), device, visitor);
                }
            }
        }
    } else {
        // Process regular children
        const auto& descriptor = param->getDescriptor();
        for (const auto& [child_name, child_desc] : descriptor.getAllSubParams()) {
            if (child_name.empty() || child_name[0] == '/') continue;
            
            Path child_path{path, child_name};
            catena::exception_with_status rc{"", catena::StatusCode::OK};
            auto sub_param = device.getParam(child_path.toString(), rc);
            
            if (rc.status == catena::StatusCode::OK && sub_param) {
                traverseParams(sub_param.get(), child_path.toString(), device, visitor);
            }
        }
    }
}

} // namespace common
} // namespace catena 