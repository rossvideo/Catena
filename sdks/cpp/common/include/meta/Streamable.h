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
 * @brief Meta programming to test if an object is streamable
 * @file Streamable.h
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * 
 * "I never metaprogram I understood. John R. Naylor, January 2024"
 */

#include <iostream>
#include <type_traits>

namespace catena {
namespace meta {

/**
 * @brief type trait to determine if a type is streamable.
 * Default is not streamable.
 */
template <typename T, typename = void>
struct is_streamable : std::false_type {};

/**
 * @brief type trait to determine if a type is streamable.
 * Types with operator DEBUG_LOG << defined are streamable.
 */
template <typename T>
struct is_streamable<T, std::void_t<decltype(std::cout << std::declval<T>())>> : std::true_type {};

/**
 * @brief Convenience function to stream a type if it is streamable.
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
