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

// lite
#include <PolyglotText.h>

#include <string>
#include <unordered_map>

namespace catena {
namespace lite {
void PolyglotText::toProto(google::protobuf::MessageLite& m) const {
    auto& dst = dynamic_cast<catena::PolyglotText&>(m);
    dst.clear_display_strings();
    for (const auto& [key, value] : display_strings_) {
        dst.mutable_display_strings()->insert({key, value});
    }
}
}  // namespace lite
}  // namespace catena