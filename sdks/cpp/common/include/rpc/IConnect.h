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
 * @file IConnect.h
 * @brief Interface for Connect.h
 * @author Ben Whitten (benjamin.whitten@rossvideo.com)
 * @copyright Copyright (c) 2025 Ross Video
 */

#pragma once

// common
#include <IParam.h>
// Proto
#include <interface/device.pb.h>
// Std
#include <string>

namespace catena {
namespace common {
 
/**
 * @brief Interface class for Connect RPCs
 */
class IConnect {
  protected:
    IConnect() = default;
    IConnect(const IConnect&) = default;
    IConnect& operator=(const IConnect&) = default;
    IConnect(IConnect&&) = default;
    IConnect& operator=(IConnect&&) = default;
    virtual ~IConnect() = default;

    /**
     * @brief Returns true if the call has been canceled.
     */
    virtual inline bool isCancelled() = 0;

    /**
     * @brief Updates the response message with parameter values and handles 
     * authorization checks.
     * 
     * @param oid - The OID of the value to update
     * @param idx - The index of the value to update
     * @param p - The parameter to update
     */
    virtual void updateResponse_(const std::string& oid, size_t idx, const IParam* p) = 0;
    
    /**
     * @brief Updates the response message with a ComponentLanguagePack and
     * handles authorization checks.
     * 
     * @param l The added ComponentLanguagePack emitted by device.
     */
    virtual void updateResponse_(const catena::DeviceComponent_ComponentLanguagePack& l) = 0;
};
 
}; // common
}; // catena
 