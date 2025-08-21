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
 * @brief Tags to differentiate collections with the same type for template specialization
 * @file Tags.h
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

#include <functional>
#include <string>

namespace catena {
namespace common {


class IConstraint; // forward declaration
class IParam; // forward declaration
class IMenu; // forward declaration
class IMenuGroup; // forward declaration
class ILanguagePack; // forward declaration

/**
 * @brief Alias for IConstraint used to improve readability
 */
struct ConstraintTag {using type = IConstraint;};
/**
 * @brief Alias for IParam used to improve readability
 */
struct ParamTag {using type = IParam;};
/**
 * @brief Alias for IParam used to improve readability
 */
struct CommandTag {using type = IParam;};
/**
 * @brief Alias for IMenu used to improve readability
 */
struct MenuTag {using type = IMenu;};
/**
 * @brief Alias for IMenuGroup used to improve readability
 */
struct MenuGroupTag {using type = IMenuGroup;};
/**
 * @brief Alias for ILanguagePack used to improve readability
 */
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
