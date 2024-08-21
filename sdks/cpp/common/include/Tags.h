#pragma once

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
