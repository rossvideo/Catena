#pragma once

#include <iostream>

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


template <typename T, typename = void>
struct is_variant : std::false_type {};

template <typename T>
struct is_variant<T, std::void_t<decltype(std::get<0>(std::declval<T>()))>> : std::true_type {};




/**
 * @brief type list
*/
template <typename... Ts>
class TypeList {};

/**
 * get the type at the front of a type list
*/
template <typename L>
class FrontT;

template <typename Head, typename... Tail>
class FrontT<TypeList<Head, Tail...>> {
  public:
    using type = Head;
};

template <typename L>
using Front = typename FrontT<L>::type;

/**
 * pop the front type off a type list
*/
template <typename L>
class PopFrontT;

template <typename Head, typename... Tail>
class PopFrontT<TypeList<Head, Tail...>> {
  public:
    using type = TypeList<Tail...>;
};

template <typename L>
using PopFront = typename PopFrontT<L>::type;

/**
 * push a type onto the front of a type list
*/
template <typename L, typename T>
class PushFrontT;

template <typename... Ts, typename T>
class PushFrontT<TypeList<Ts...>, T> {
  public:
    using type = TypeList<T, Ts...>;
};

template <typename L, typename T>
using PushFront = typename PushFrontT<L, T>::type;

/**
 * get the Nth type in a type list
*/
template <typename L, unsigned int N>
class NthElementT : public NthElementT<PopFront<L>, N-1> {};

template <typename L>
class NthElementT<L, 0> : public FrontT<L> {};

template <typename L, unsigned int N>
using NthElement = typename NthElementT<L, N>::type;




}  // namespace meta
}  // namespace catena