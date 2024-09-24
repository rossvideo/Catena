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
 * enabled if T is a vector of struct with getStructInfo method
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

}  // namespace lite
}  // namespace catena

