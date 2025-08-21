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
 * @file ILanguagePack.h
 * @brief Interface class for language packs
 * @author John R. Naylor, john.naylor@rossvideo.com
 * @date 2024-08-22
 * @copyright Copyright (c) 2024 Ross Video
 */

// protobuf interface
#include <interface/language.pb.h>

#include <string>
#include <unordered_map>

namespace catena {
namespace common {

/**
 * @brief Interface class for Language Packs
 */
class ILanguagePack {
public:
    /**
     * @brief Const iterator to the key/word pairs
     */
    using const_iterator = std::unordered_map<std::string, std::string>::const_iterator;

    ILanguagePack() = default;
    ILanguagePack(ILanguagePack&&) = default;
    ILanguagePack& operator=(ILanguagePack&&) = default;

    /**
     * @brief Destructor
     */
    virtual ~ILanguagePack() = default;

    /**
     * @brief Deserializes a language pack from a protobuf message
     * @param pack The protobuf message
     */
    virtual void fromProto(const catena::LanguagePack& pack) = 0;

    /**
     * @brief Serializes a language pack to a protobuf message
     * @param pack The protobuf message
     */
    virtual void toProto(catena::LanguagePack& pack) const = 0;

    /**
     * @brief Gets the begin iterator to the key/word pairs
     */
    virtual inline const_iterator begin() const = 0;

    /**
     * @brief Gets the end iterator to the key/word pairs
     */
    virtual inline const_iterator end() const = 0;
};

}  // namespace common
}  // namespace catena
