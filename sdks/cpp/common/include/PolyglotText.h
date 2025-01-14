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
 * @file PolyglotText.h
 * @brief Polyglot Text serialization and deserialization to protobuf
 * @author John R. Naylor
 * @date 2024-07-07
 */

// common
#include <IPolyglotText.h>

// protobuf interface
#include <interface/language.pb.h>

#include <string>
#include <unordered_map>
#include <initializer_list>

namespace catena {
namespace common {


/**
 * @brief Implementation of multilingual text handling with serialization support.
 *
 * Provides an implementation of the IPolyglotText interface. Manages multilingual display 
 * strings stored as language-to-text mappings and supports serialization into protocol buffers. 
 */

class PolyglotText : public IPolyglotText {
  public:
    using DisplayStrings = std::unordered_map<std::string, std::string>;

  public:
    PolyglotText(const DisplayStrings& display_strings) : display_strings_(display_strings) {}
    PolyglotText() = default;
    PolyglotText(PolyglotText&&) = default;
    PolyglotText& operator=(PolyglotText&&) = default;
    virtual ~PolyglotText() = default;

    // Constructor from initializer list
    PolyglotText(ListInitializer list)
      : display_strings_(list.begin(), list.end()) {}


    void toProto(google::protobuf::MessageLite& dst) const override;

    inline const DisplayStrings& displayStrings() const override { return display_strings_; }

  private:
    DisplayStrings display_strings_;
};
}  // namespace common
}  // namespace catena