#pragma once

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
 * @file IParamVisitor.h
 * @brief Implements Catena IParamVisitor
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-04-02
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <IParam.h>
#include <Path.h>
#include <Device.h>

namespace catena {
namespace common {

/**
 * @brief Interface for parameter visitors
 */
class IParamVisitor {
    public:
        /**
         * @brief Virtual destructor
         */
        virtual ~IParamVisitor() = default;
        
        /**
         * @brief Visit a parameter
         * @param param The parameter to visit
         * @param path The path of the parameter
         */
        virtual void visit(IParam* param, const std::string& path) = 0;
        
        /**
         * @brief Visit an array
         * @param param The array to visit
         * @param path The path of the array
         * @param length The length of the array
         */
        virtual void visitArray(IParam* param, const std::string& path, uint32_t length) = 0;
};

} // namespace common
} // namespace catena 