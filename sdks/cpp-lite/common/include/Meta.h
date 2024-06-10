#pragma once

/**
 * @brief Meta programming utilities
 * @file Meta.h
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * 
 * "I never metaprogram I understood. John R. Naylor, January 2024"
 * 
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

#include <iostream>
#include <variant>

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

/**
 * @brief Struct that holds a list of types that can be manipulated at compile time
*/
template <typename... Ts>
class TypeList {};

/**
 * get the type at the front of a type list, forward declaration
*/
template <typename L>
class FrontT;

/**
 * get the type at the front of a type list
*/
template <typename Head, typename... Tail>
class FrontT<TypeList<Head, Tail...>> {
  public:
    using type = Head;
};

/**
 * @brief convenience alias for FrontT
*/
template <typename L>
using Front = typename FrontT<L>::type;

/**
 * pop the front type off a type list, forward declaration
*/
template <typename L>
class PopFrontT;

/**
 * pop the front type off a type list
*/
template <typename Head, typename... Tail>
class PopFrontT<TypeList<Head, Tail...>> {
  public:
    using type = TypeList<Tail...>;
};

/**
 * convenience alias for PopFrontT
*/
template <typename L>
using PopFront = typename PopFrontT<L>::type;

/**
 * push a type onto the front of a type list, forward declaration
*/
template <typename L, typename T>
class PushFrontT;

/**
 * @brief push a type onto the front of a type list
*/
template <typename... Ts, typename T>
class PushFrontT<TypeList<Ts...>, T> {
  public:
    using type = TypeList<T, Ts...>;
};

/**
 * @brief convenience alias for PushFrontT
*/
template <typename L, typename T>
using PushFront = typename PushFrontT<L, T>::type;

/**
 * get the Nth type in a type list
*/
template <typename L, unsigned int N>
class NthElementT : public NthElementT<PopFront<L>, N-1> {};

/**
 * get the 0th type in a type list, terminates recursion
*/
template <typename L>
class NthElementT<L, 0> : public FrontT<L> {};

/**
 * get the Nth type in a type list, recursive
*/
template <typename L, unsigned int N>
using NthElement = typename NthElementT<L, N>::type;

}  // namespace meta
}  // namespace catena