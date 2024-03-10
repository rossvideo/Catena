#pragma once

/**
 * @brief A collection of translation methods between C++ types and Catena::Value
 * @file ValueAccessors.h
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

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

#include <param.pb.h>

#include <Functory.h>
#include <Meta.h>
#include <TypeTraits.h>

#include <typeindex>
#include <typeinfo>
#include <variant>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <type_traits>

namespace catena {

/**
 * @brief type for indexing into values that are arrays
 *
 */
using ValueIndex = uint32_t;

/**
 * @brief index value used to trigger special behaviors.
 *
 * getValue with this value specified as the index will get all values of an
 * array. setValue will append the value to the array
 *
 */
static constexpr ValueIndex kValueEnd = ValueIndex(-1);

/**
 * @brief Value::KindCase looker upper
 */
template <typename V = float> catena::Value::KindCase getKindCase(meta::TypeTag<V> = {}) {
    if constexpr (has_getStructInfo<V>) {
        return catena::Value::KindCase::kStructValue;
    }
    return catena::Value::KindCase::KIND_NOT_SET;
}

/**
 * Populates the various Functories with methods to set and get values from the protobuf based 
 * messages.
 * 
 * Call this method once at startup to initialize the Functories.
*/
void initValueAccessors();

/**
 * @brief type alias to Functory of methods to set catena::Value from C++ scalar types
 */
using Setter = catena::patterns::Functory<catena::Value::KindCase, void, catena::Value*, const void*>;

/**
 * @brief type alias to Functory of methods to get C++ scalar types from catena::Value
 */
using Getter = catena::patterns::Functory<catena::Value::KindCase, void, void*, const catena::Value*>;

/**
 * @brief type alias to Functory of methods to set catena::Value from C++ vector types
 */
using SetterAt = catena::patterns::Functory<catena::Value::KindCase, void, catena::Value*, const void*,
                                            const ValueIndex>;

/**
 * @brief type alias to Functory of methods to get C++ vector types from catena::Value
 */
using GetterAt = catena::patterns::Functory<catena::Value::KindCase, void, void*, const catena::Value*,
                                            const ValueIndex>;

/**
 * @brief type alias for Functory of methods to get VariantInfo given its type_index
 */
using VariantInfoGetter = catena::patterns::Functory<std::type_index, const VariantInfo&>;

/**
 * @brief type alias for the function that gets values from the protobuf based device model for delivery to attached
 * clients
 */
using ValueGetter =
    catena::patterns::Functory<catena::Value::KindCase, void, catena::Value*, const catena::Value&, ValueIndex>;

/**
 * @brief type alias for the function that sets values in the protobuf device model in response to client requests
 */
using ValueSetter =
    catena::patterns::Functory<catena::Value::KindCase, void, catena::Value&, const catena::Value&, ValueIndex>;


} // namespace catena