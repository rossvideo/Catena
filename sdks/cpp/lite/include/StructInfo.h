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
//

/**
 * @file StructInfo.h
 * @brief Structured data serialization and deserialization to protobuf
 * @author John R. Naylor
 * @author John Danen
 * @date 2024-07-07
 */

// common
#include <meta/Typelist.h>
#include <meta/IsVector.h>

// lite
#include <AuthzInfo.h>

// protobuf interface
#include <interface/param.pb.h>

#include <string>
#include <cstddef>
#include <vector>
#include <utility>

namespace catena {
namespace lite {

template <typename T>
struct StructInfo{};

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
    using Field = FieldType;
    const char* name; /*< the name of the field */
    FieldType StructType::* memberPtr; /*< the field's offset from the base of its enclosing struct */


    constexpr FieldInfo(const char* n, FieldType StructType::* m)
        : name(n), memberPtr(m) {}
};

template <typename T>
void toProto(catena::Value& dst, const T* src, const AuthzInfo& auth);

/**
 * Free standing method to stream an entire array of structured data to protobuf
 * 
 * enabled if T is a vector of CatenaStruct
 * 
 * @tparam T the type of the value
 */
template <CatenaStructArray T>
void toProto(catena::Value& dst, const T* src, const AuthzInfo& auth) {
    using structType = T::value_type;
    auto* dstArray = dst.mutable_struct_array_values();
    
    for (const auto& item : *src) {
        catena::Value elemValue;
        toProto(elemValue, &item, auth);
        *dstArray->add_struct_values() = *elemValue.mutable_struct_value();
    }
}

/**
 * Free standing method to stream structured data to protobuf
 * 
 * enabled if T has matches the CatenaStruct concept
 * 
 * @tparam T the type of the value
 */
template <CatenaStruct T>
void toProto(catena::Value& dst, const T* src, const AuthzInfo& auth) {
    auto fields = StructInfo<T>::fields;
    auto* dstFields = dst.mutable_struct_value()->mutable_fields();

    auto addField = [&](const auto& field) {
        AuthzInfo subParamAuth = auth.subParamInfo(field.name);
        if (subParamAuth.readAuthz()) {
            catena::Value* newFieldValue = (*dstFields)[field.name].mutable_value();
            toProto(*newFieldValue, &(src->*(field.memberPtr)), subParamAuth);
        }
    };

    std::apply([&](auto... field) {
        (addField(field), ...);
    }, fields);
}

template <typename T>
void fromProto(const catena::Value& src, T* dst, const AuthzInfo& auth);

template <CatenaStructArray T>
void fromProto(const catena::Value& src, T* dst, const AuthzInfo& auth) {
    using structType = T::value_type;
    auto& srcArray = src.struct_array_values().struct_values();
    dst->clear();
    
    for (int i = 0; i < srcArray.size(); ++i) {
        catena::Value item;
        *item.mutable_struct_value() = srcArray.Get(i);
        structType& elemValue = dst->emplace_back();
        fromProto(item, &elemValue, auth);
    }
}

template <CatenaStruct T>
void fromProto(const catena::Value& src, T* dst, const AuthzInfo& auth) {
    auto fields = StructInfo<T>::fields;
    auto& srcFields = src.struct_value().fields();

    /**
     * @todo implement error handling
     */
    auto writeField = [&](const auto& field) {
        AuthzInfo subParamAuth = auth.subParamInfo(field.name);
        if (subParamAuth.writeAuthz()) {
            const catena::Value& fieldValue = srcFields.at(field.name).value();
            fromProto(fieldValue, &(dst->*(field.memberPtr)), subParamAuth);
        }
    };

    std::apply([&](auto... field) {
        (writeField(field), ...);
    }, fields);
}

}  // namespace lite
}  // namespace catena

