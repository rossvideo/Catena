#pragma once

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
 * @file IPolyglotText.h
 * @brief Catena's multi-language text interface
 * @author John R. Naylor, john.naylor@rossvideo.com
 * @copyright Copyright (c) 2024 Ross Video
 */

// Protobuf
#include <interface/language.pb.h>

#include <unordered_map>
#include <string>
#include <initializer_list>

namespace catena {
namespace common {

/**
 * @brief Interface for managing multilingual text with support for
 * serialization to protocol buffers.
 *
 * Defines the contract for handling and accessing multilingual display strings
 * and serializing them into protocol buffers.
 */
class IPolyglotText {
  public:
    using DisplayStrings = std::unordered_map<std::string, std::string>;
    using ListInitializer = std::initializer_list<std::pair<std::string, std::string>>;
  
    IPolyglotText() = default;
    IPolyglotText(IPolyglotText&&) = default;
    IPolyglotText& operator=(IPolyglotText&&) = default;
    virtual ~IPolyglotText() = default;

    /**
     * @brief Serializes the multilingual text into a protobuf message.
     * @param dst The destination protobuf object.
     */
    virtual void toProto(st2138::PolyglotText& dst) const = 0;
    /**
     * @brief Returns the unordered map of multilingual text.
     */
    virtual const DisplayStrings& displayStrings() const = 0;
};

}  // namespace common
}  // namespace catena
