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
 * @file StructInfo.h
 * @brief Structured data serialization and deserialization to protobuf
 * @author John R. Naylor
 * @date 2024-07-07
 */

// common
#include <meta/Typelist.h>
#include <meta/IsVector.h>

// protobuf interface
#include <interface/param.pb.h>

#include <string>
#include <cstddef>
#include <vector>

namespace catena {
namespace lite {

template <typename T>
struct Reflect{};

template <typename T>
concept CatenaStruct = requires {
    T::isCatenaStruct();
};

template <typename T>
concept CatenaStructArray = meta::IsVector<T> && CatenaStruct<typename T::value_type>;

/**
 * @brief FieldInfo is a struct that contains information about a field in a struct
 *
 */
template <typename FieldType, CatenaStruct StructType>
struct FieldInfo {
    const char* name; /*< the name of the field */
    const FieldType StructType::* memberPtr; /*< the field's offset from the base of its enclosing struct */

    constexpr FieldInfo(const char* n, const FieldType StructType::* m)
        : name(n), memberPtr(m) {}

    // /**
    //  * @brief a function that converts a field to a protobuf value
    //  *
    //  * @param dst the destination protobuf value
    //  * @param src the source value
    //  * 
    //  * The code generator points this to the appropriate toProto method
    //  * for the field's type
    //  * 
    //  */
    // std::function<void(catena::Value& dst, const void* src)> toProto;

    // /**
    //  * @brief a function that converts a protobuf value to a field
    //  *
    //  * @param dst the destination value
    //  * @param src the source protobuf value
    //  * 
    //  * The code generator points this to the appropriate fromProto method
    //  * for the field's type
    //  * 
    //  */
    // std::function<void(void* dst, const catena::Value& src)> fromProto;
};

/**
 * @brief StructInfo is a struct that contains information about a struct defined 
 * in the Catena device model.
 *
 */

// struct StructInfo {
//     std::string name; /*< the struct's type name */
//     // std::vector<FieldInfo> fields; /*< information about its fields */
// };



/**
 * Free standing method to stream an entire array of structured data to protobuf
 * 
 * enabled if T is a vector of struct with getStructInfo method
 * 
 * @tparam T the type of the value
 */
// template <CatenaStructArray T>
// void toProto(catena::Value& dst, const void* src){
//     using structType = T::value_type;
//     const auto* srcArray = reinterpret_cast<const T*>(src);
//     const auto& si = structType::getStructInfo();

//     catena::StructList* dstArray = dst.mutable_struct_array_values();
//     for (const structType& element : *srcArray){
//         catena::StructValue* dstElement = dstArray->add_struct_values();
//         ::google::protobuf::Map<std::string, ::catena::StructField>* dstFields = dstElement->mutable_fields();
//         for (const auto& field : si.fields) {
//             auto& dstField = (*dstFields)[field.name];
//             auto& dstValue = *dstField.mutable_value();

//             // required so that pointer math works correctly
//             const char* src_ptr = reinterpret_cast<const char*>(&element);
//             field.toProto(dstValue, src_ptr + field.offset);
//         }
//     }
// }


template <typename T>
void toProto(catena::Value& dst, const T* src);

/**
 * Free standing method to stream structured data to protobuf
 * 
 * enabled if T has a getStructInfo method
 * 
 * @tparam T the type of the value
 */
template <CatenaStruct T>
void toProto(catena::Value& dst, const T* src) {
    auto fields = Reflect<T>::fields;
    auto* dstFields = dst.mutable_struct_value()->mutable_fields();
    std::apply([&](auto... field) {
        (toProto(*(*dstFields)[field.name].mutable_value(), &(src->*(field.memberPtr))), ...);
    }, fields);
}

/**
 * Free standing method to stream protobuf to structured data
 * 
 * enabled if T has a getStructInfo method
 * 
 * @tparam T the type of the value
 */
template <CatenaStruct T>
void fromProto(void* dst, const catena::Value& src) {
    // const auto& si = T::getStructInfo();

    // // required so that pointer math works correctly
    // char* dst_ptr = reinterpret_cast<char*>(dst);

    // deserialize each field
    // for (const auto& field : si.fields) {
    //     const ::catena::StructValue& structValue = src.struct_value();
    //     const ::google::protobuf::Map<std::string, ::catena::StructField>& srcFields = structValue.fields();
    //     const auto& srcField = srcFields.at(field.name);
    //     const auto& srcValue = srcField.value();
    //     field.fromProto(dst_ptr + field.offset, srcValue);
    // }
}

template <CatenaStructArray T>
void fromProto(void* dst, const catena::Value& src){
    
}

template <typename T>
void fromProto(void* dst, const catena::Value& src);

}  // namespace lite
}  // namespace catena

