#pragma once

#include <common/include/meta/Typelist.h>
#include <lite/param.pb.h>

#include <string>
#include <cstddef>
#include <vector>

namespace catena {
namespace lite {

/**
 * @brief FieldInfo is a struct that contains information about a field in a struct
 *
 */
struct FieldInfo {
    std::string name; /*< the name of the field */
    std::size_t offset; /*< the field's offset from the base of its enclosing struct */

    /**
     * @brief a function that converts a field to a protobuf value
     *
     * @param dst the destination protobuf value
     * @param src the source value
     * 
     * The code generator points this to the appropriate toProto method
     * for the field's type
     * 
     */
    std::function<void(catena::Value&,const void*)> toProto;
};

/**
 * @brief StructInfo is a struct that contains information about a struct defined 
 * in the Catena device model.
 *
 */
struct StructInfo {
    std::string name; /*< the struct's type name */
    std::vector<FieldInfo> fields; /*< information about its fields */
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

}  // namespace meta

namespace lite {

/**
 * Free standing method to stream structured data to protobuf
 * 
 * enabled if T has a getStructInfo method
 * 
 * @tparam T the type of the value
 */
template <typename T>
typename std::enable_if<meta::has_getStructInfo<T>, void>::type toProto(catena::Value& dst, const void* src) {
    const auto& si = T::getStructInfo();

    // required so that pointer math works correctly
    const char* src_ptr = reinterpret_cast<const char*>(src);

    // serialize each field
    for (const auto& field : si.fields) {
        ::catena::StructValue* structValue = dst.mutable_struct_value();
        ::google::protobuf::Map<std::string, ::catena::StructField>* dstFields = structValue->mutable_fields();
        auto& dstField = (*dstFields)[field.name];
        auto& dstValue = *dstField.mutable_value();
        field.toProto(dstValue, src_ptr + field.offset);
    }
}


template <typename T>
typename std::enable_if<!meta::has_getStructInfo<T>, void>::type toProto(catena::Value& value, const void* src);



}  // namespace lite
}  // namespace catena

