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
 * @brief Meta programming to test if an object is a std::variant
 * @file Variant.h
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

#include <type_traits> // std::void_t
#include <variant>     // std::get

namespace catena {
namespace meta {

/**
 * @brief type trait to determine if a type is a std::variant
 * default is false
*/
template <typename T, typename = void>
struct is_variant : std::false_type {};

/**
 * @brief type trait to determine if a type is a std::variant
 * those with std::get<0> defined are std::variants
*/
template <typename T>
struct is_variant<T, std::void_t<decltype(std::get<0>(std::declval<T>()))>> : std::true_type {};
} // namespace meta
} // namespace catena