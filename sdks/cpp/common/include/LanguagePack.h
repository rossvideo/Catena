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
 * @file LanguagePack.h
 * @brief Implements multi language support class LanguagePack
 * @author John R. Naylor john.naylor@rossvideo.com
 * @date 2024-08-22
 * @copyright Copyright (c) 2024 Ross Video
 */

//common
#include <ILanguagePack.h>
#include <IDevice.h>

// std
#include <initializer_list>

namespace catena {
namespace common {

/**
 * @brief A class represeting a language via a set of key/word pairs with
 * support for protobuf serialization.
 */
class LanguagePack : public common::ILanguagePack {
  public:
    /**
     * @brief A list of words in a language, used by the main constructor
     */
    using ListInitializer = std::initializer_list<std::pair<std::string, std::string>>;

    LanguagePack() = delete;
    /**
     * @brief LanguagePack does not have copy semantics
     */
    LanguagePack(const LanguagePack&) = delete;
    LanguagePack& operator=(const LanguagePack&) = delete;

    /**
     * @brief LanguagePack has move semantics
     */
    LanguagePack(LanguagePack&&) = default;
    LanguagePack& operator=(LanguagePack&&) = default;

    /**
     * @brief Destructor
     */
    virtual ~LanguagePack() = default;

    /**
     * @brief Constructs a language pack from a list of words
     * @param languageCode The language's unique id (e.g. "es" for Spanish)
     * @param name The name of the language
     * @param list The list of key/word pairs
     * @param dev The device model this language pack belongs to
     */
    LanguagePack(const std::string& languageCode, const std::string& name, ListInitializer list, IDevice& dev);

    /**
     * @brief Deserializes a language pack from a protobuf message
     * @param pack The protobuf message
     */
    void fromProto(const ::catena::LanguagePack& pack) override;

    /**
     * @brief Serializes a language pack to a protobuf message
     * @param pack The protobuf message
     */
    void toProto(::catena::LanguagePack& pack) const override;

    /**
     * @brief Gets the begin iterator to the key/word pairs
     */
    inline const_iterator begin() const override { return words_.cbegin(); }

    /**
     * @brief Gets the end iterator to the key/word pairs
     */
    inline const_iterator end() const override { return words_.cend(); }

  private:
    /**
     * @brief The language's name (e.g. "Spanish") 
     */
    std::string name_;
    /**
     * @brief The map of key/word pairs.
     */
    std::unordered_map<std::string, std::string> words_;
};

}  // namespace common
}  // namespace catena
