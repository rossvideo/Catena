#pragma once

// Licensed under the Creative Commons Attribution NoDerivatives 4.0
// International Licensing (CC-BY-ND-4.0);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
// https://creativecommons.org/licenses/by-nd/4.0/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

/**
 * @file IParam.h
 * @brief Interface for parameters
 * @author John R. Naylor, john.naylor@rossvideo.com
 * @date 2024-08-22
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <string>
#include <unordered_map>

namespace catena {

class LanguagePack; // forward reference

namespace common {

/**
 * @brief Interface for Language Packs
 */
class ILanguagePack {
public:
    /**
     * @brief Const iterator to the key/word pairs
     */
    using const_iterator = std::unordered_map<std::string, std::string>::const_iterator;

public:
    ILanguagePack() = default;
    ILanguagePack(ILanguagePack&&) = default;
    ILanguagePack& operator=(ILanguagePack&&) = default;
    virtual ~ILanguagePack() = default;

    /**
     * @brief serialize a language pack to a protobuf message
     * @param pack the protobuf message
     */
    virtual void toProto(catena::LanguagePack& pack) const = 0;

    /**
     * @brief deserialize a language pack from a protobuf message
     * @param pack the protobuf message
     */
    virtual void fromProto(const catena::LanguagePack& pack) = 0;

    /**
     * get the begin iterator to the key/word pairs
     */
    virtual const_iterator begin() const = 0;

    /**
     * get the end iterator to the key/word pairs
     */
    virtual const_iterator end() const = 0;
};
}  // namespace common
}  // namespace catena
