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
 * @file IPolyglotText.h
 * @brief Catena's multi-language text interface
 * @author John R. Naylor, john.naylor@rossvideo.com
 * @copyright Copyright (c) 2024 Ross Video
 */

#include "google/protobuf/message_lite.h" 

#include <unordered_map>
#include <string>

namespace catena {
namespace common {
class IPolyglotText {
  public:
    using DisplayStrings = std::unordered_map<std::string, std::string>;
    using ListInitializer = std::initializer_list<std::pair<std::string, std::string>>;
  public:
    IPolyglotText() = default;
    IPolyglotText(IPolyglotText&&) = default;
    IPolyglotText& operator=(IPolyglotText&&) = default;
    virtual ~IPolyglotText() = default;

    virtual void toProto(google::protobuf::MessageLite& msg) const = 0;

    virtual const DisplayStrings& displayStrings() const = 0;

};
}  // namespace common
}  // namespace catena