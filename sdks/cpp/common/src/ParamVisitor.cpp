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

#include <ParamVisitor.h>
#include <exception>

namespace catena {
namespace common {

// Traverses the parameters of a device and visits each parameter using the visitor
void ParamVisitor::traverseParams(IParam* param, const std::string& path, IDevice& device, IParamVisitor& visitor, const IAuthorizer& authz) {
    if (!param) return;

    // First visit the current parameter itself
    visitor.visit(param, path);
    
    // Special handling for array-type parameters
    if (param->isArrayType()) {
        uint32_t array_length = param->size();
        if (array_length > 0) {
            // Notify visitor about array properties (e.g., length)
            visitor.visitArray(param, path, array_length);
            
            // Only process array elements if we're not already inside an array element
            for (uint32_t i = 0; i < array_length; i++) {
                // Create path for this array element (e.g., "/params/array/0")
                Path indexed_path(path);
                indexed_path.push_back(std::to_string(i));
                catena::exception_with_status rc{"", catena::StatusCode::OK};
                
                // Get the parameter for this array index
                auto indexed_param = device.getParam(indexed_path.toString(true), rc, authz);
                if (indexed_param && authz.readAuthz(*indexed_param)) {
                    // Recursively process this array element and all its children
                    traverseParams(indexed_param.get(), indexed_path.toString(true), device, visitor, authz);
                }
            }
        }
    }
    
    // Process all regular (non-array-element) children of this parameter
    const auto& descriptor = param->getDescriptor();
    if (descriptor.getAllSubParams().empty()) {
        return;  // No children to traverse
    }
    
    
    for (const auto& [child_name, child_desc] : descriptor.getAllSubParams()) {
        // Skip invalid child names (empty or absolute paths)
        if (child_name.empty() || child_name[0] == '/') continue;
        
        // Construct the full path for this child
        Path child_path(path);
        child_path.push_back(child_name);
        catena::exception_with_status rc{"", catena::StatusCode::OK};
        
        // Get the child parameter
        auto sub_param = device.getParam(child_path.toString(true), rc, authz);
        
        // If child exists and we can access it, process it recursively
        if (rc.status == catena::StatusCode::OK && sub_param && authz.readAuthz(*sub_param)) {
            traverseParams(sub_param.get(), child_path.toString(true), device, visitor, authz);
        }
    }
}

} // namespace common
} // namespace catena 