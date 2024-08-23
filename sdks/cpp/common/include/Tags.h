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
 * @brief Tags to differentiate collections with the same type for template specialization
 * @file Tags.h
 * @copyright Copyright © 2023 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

#include <functional>
#include <string>

namespace catena {
namespace common {


class IConstraint; // forward declaration
class IParam; // forward declaration
class IMenuGroup; // forward declaration
class ILanguagePack; // forward declaration

struct ConstraintTag {using type = IConstraint;};
struct ParamTag {using type = IParam;};
struct CommandTag {using type = IParam;};
struct MenuGroupTag {using type = IMenuGroup;};
struct LanguagePackTag {using type = ILanguagePack;};

template<typename TAG>
using AddItem = std::function<void(const std::string& key, typename TAG::type* item)>;

} // namespace common
} // namespace catena

/**
 * @def GET_ITEM(ITEM_TAG, MAP)
 * @brief gets an item from one of the collections owned by the device
 * @return nullptr if the item is not found, otherwise the item
 * @param ITEM_TAG identifies the collection
 * @param MAP the collection to search
 */
#define GET_ITEM(ITEM_TAG, MAP) \
if constexpr(std::is_same_v<ITEM_TAG, TAG>) { \
    auto it = MAP.find(key); \
    return it == MAP.end() ? nullptr : it->second; \
}
