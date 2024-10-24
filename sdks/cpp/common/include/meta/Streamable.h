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
 * @brief Meta programming to test if an object is streamable
 * @file Streamable.h
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * 
 * "I never metaprogram I understood. John R. Naylor, January 2024"
 * 
 */

#include <iostream>
#include <type_traits>

namespace catena {
namespace meta {

/**
 * @brief type trait to determine if a type is streamable
 * default is not streamable.
*/
template <typename T, typename = void>
struct is_streamable : std::false_type {};

/**
 * @brief type trait to determine if a type is streamable
 * types with operator std::cout << defined are streamable.
*/
template <typename T>
struct is_streamable<T, std::void_t<decltype(std::cout << std::declval<T>())>> : std::true_type {};

/**
 * convenience function to stream a type if it is streamable
*/
template <typename T>
std::ostream& stream_if_possible(std::ostream& os, const T& data) {
    if constexpr (catena::meta::is_streamable<T>::value) {
        os << data;
    } else {
        os << "is not streamable";
    }
    return os;
}
} // namespace meta
} // namespace catena