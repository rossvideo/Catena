#pragma once

/**
 * @file StructInfo.h
 * @brief Structured data serialization and deserialization to protobuf
 * @author John R. Naylor
 * @date 2024-07-07
 */

#include <interface/param.pb.h>
#include <common/include/meta/Typelist.h>

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
    std::function<void(catena::Value& dst, const void* src)> toProto;

    /**
     * @brief a function that converts a protobuf value to a field
     *
     * @param dst the destination value
     * @param src the source protobuf value
     * 
     * The code generator points this to the appropriate fromProto method
     * for the field's type
     * 
     */
    std::function<void(void* dst, const catena::Value& src)> fromProto;
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

template <typename T>
concept CatenaStruct = requires {
    T::getStructInfo();
};

namespace lite {

/**
 * Free standing method to stream structured data to protobuf
 * 
 * enabled if T has a getStructInfo method
 * 
 * @tparam T the type of the value
 */
template <CatenaStruct T>
void toProto(catena::Value& dst, const void* src) {
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
void toProto(catena::Value& dst, const void* src);


/**
 * Free standing method to stream protobuf to structured data
 * 
 * enabled if T has a getStructInfo method
 * 
 * @tparam T the type of the value
 */
template <CatenaStruct T>
void fromProto(void* dst, const catena::Value& src) {
    const auto& si = T::getStructInfo();

    // required so that pointer math works correctly
    char* dst_ptr = reinterpret_cast<char*>(dst);

    // deserialize each field
    for (const auto& field : si.fields) {
        const ::catena::StructValue& structValue = src.struct_value();
        const ::google::protobuf::Map<std::string, ::catena::StructField>& srcFields = structValue.fields();
        const auto& srcField = srcFields.at(field.name);
        const auto& srcValue = srcField.value();
        field.fromProto(dst_ptr + field.offset, srcValue);
    }
}

template <typename T>
void fromProto(void* dst, const catena::Value& src);

}  // namespace lite
}  // namespace catena

