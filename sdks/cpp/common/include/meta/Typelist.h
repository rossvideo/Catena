#pragma once

/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * @brief Type list meta programming
 * @file TypeList.h
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 *
 * "I never metaprogram I understood. John R. Naylor, January 2024"
 *
 */

namespace catena {
namespace meta {

/**
 * @brief Struct that holds a list of types that can be manipulated at compile time
 */
template <typename... Ts> class TypeList {};

/**
 * @brief Get the type at the front of a type list, forward declaration
 */
template <typename L> class FrontT;

/**
 * @brief Get the type at the front of a type list
 */
template <typename Head, typename... Tail> class FrontT<TypeList<Head, Tail...>> {
  public:
    using type = Head;
};

/**
 * @brief Convenience alias for FrontT
 */
template <typename L> using Front = typename FrontT<L>::type;

/**
 * @brief Pop the front type off a type list, forward declaration
 */
template <typename L> class PopFrontT;

/**
 * @brief Pop the front type off a type list
 */
template <typename Head, typename... Tail> class PopFrontT<TypeList<Head, Tail...>> {
  public:
    using type = TypeList<Tail...>;
};

/**
 * @brief Convenience alias for PopFrontT
 */
template <typename L> using PopFront = typename PopFrontT<L>::type;

/**
 * @brief Push a type onto the front of a type list, forward declaration
 */
template <typename L, typename T> class PushFrontT;

/**
 * @brief push a type onto the front of a type list
 */
template <typename... Ts, typename T> class PushFrontT<TypeList<Ts...>, T> {
  public:
    using type = TypeList<T, Ts...>;
};

/**
 * @brief convenience alias for PushFrontT
 */
template <typename L, typename T> using PushFront = typename PushFrontT<L, T>::type;

/**
 * @brief get the Nth type in a type list
 */
template <typename L, unsigned int N> class NthElementT : public NthElementT<PopFront<L>, N - 1> {};

/**
 * @brief get the 0th type in a type list, terminates recursion
 */
template <typename L> class NthElementT<L, 0> : public FrontT<L> {};

/**
 * @brief get the Nth type in a type list, recursive
 */
template <typename L, unsigned int N> using NthElement = typename NthElementT<L, N>::type;

}  // namespace meta
}  // namespace catena