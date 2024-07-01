#pragma once

#include <common/include/meta/Typelist.h>

#include <string>
#include <cstddef>
#include <vector>

namespace catena {
namespace lite {
struct FieldInfo {
    std::string name;
    std::size_t offset;
};

struct StructInfo {
    std::string name;
    std::vector<FieldInfo> fields;
};

}  // namespace lite

namespace meta {
    /**
 * @brief determine at compile time if a type T has a getStructInfo method.
 *
 * default is false
 *
 * @todo refactor using C++ 20 concepts to make the code a bit clearer
 *
 * @tparam T
 * @tparam typename
 */
template <typename T, typename = void> constexpr bool has_getStructInfo{};

/**
 * @brief specialization for types that do have getStructInfo method
 *
 * @tparam T
 */
template <typename T>
constexpr bool has_getStructInfo<T, std::void_t<decltype(std::declval<T>().getStructInfo())>> = true;

}
}  // namespace catena

