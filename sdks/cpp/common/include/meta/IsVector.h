#pragma once

#include <vector>
#include <type_traits>

namespace catena {
namespace meta {
    /**
     * @brief Concept to check if a type is a vector
     */
    template <typename T>
    concept IsVector = requires {
        typename std::enable_if_t<std::is_same_v<T, std::vector<typename T::value_type, typename T::allocator_type>>>;
    };
}
}




