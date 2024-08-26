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
namespace lite {
class PolyglotText : public catena::common::IPolyglotText {
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
}  // namespace lite
}  // namespace catena