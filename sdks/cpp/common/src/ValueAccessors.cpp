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

#include <ValueAccessors.h>
#include <Status.h>


#include <sstream>
#include <exception>
#include <algorithm>
#include <vector>
#include <iostream>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <variant>

using catena::ValueIndex;
using VP = catena::Value *;  // shared value pointer
using catena::Param;    // this is the protobuf Param message
using catena::Value;    // this is the protobuf Value message
using KindCase = Value::KindCase;
using catena::meta::TypeTag;

/**
 * @brief A macro used to add getter functions for simple data types
*/
#define REGISTER_GETTER(kind_case, get_method, cast) \
getter.addFunction( kind_case, [](void *dstAddr, const Value *src) { \
    *reinterpret_cast<cast *>(dstAddr) = src->get_method(); \
})

/**
 * @brief A macro used to add setter functions for simple data types
*/
#define REGISTER_SETTER(kind_case, set_method, cast) \
setter.addFunction( kind_case, [](Value *dst, const void *srcAddr) { \
    dst->set_method(*reinterpret_cast<const cast*>(srcAddr));  \
})

/**
 * @brief A macro used to add getter functions for array of simple data types
*/
#define REGISTER_ARRAY_GETTER(kind_case, cast, array_values_method, access_method) \
getter.addFunction(kind_case, [](void *dstAddr, const Value *src) { \
    auto *dst = reinterpret_cast<std::vector<cast>*>(dstAddr); \
    dst->clear(); \
    auto &arr = src->array_values_method().access_method(); \
    for (auto it = arr.begin(); it != arr.end(); ++it) { \
        dst->push_back(*it); \
    } \
})

/**
 * @brief A macro used to add setter functions for array of simple data types
*/
#define REGISTER_ARRAY_SETTER(kind_case, mutable_array_method, clear_method, cast, add_method) \
setter.addFunction(kind_case, [](catena::Value *dst, const void *srcAddr) { \
    dst->mutable_array_method()->clear_method(); \
    auto *src = reinterpret_cast<const std::vector<cast> *>(srcAddr); \
    for (auto &it : *src) { \
        dst->mutable_array_method()->add_method(it); \
    } \
})

/**
 * @brief A macro used to add getter functions for element of array of simple data types
*/
#define REGISTER_ARRAY_GETTER_AT(kind_case, cast, array_values_method, size_method, access_method) \
getterAt.addFunction(kind_case, \
    [](void *dstAddr, const Value *src, const catena::ValueIndex idx) { \
        auto *dst = reinterpret_cast<cast *>(dstAddr); \
        if (idx >= src->array_values_method().size_method()) { \
            /* range error  */ \
            std::stringstream err; \
            err << "array index is out of bounds, " << idx \
                << " >= " << src->array_values_method().size_method(); \
            BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE); \
        } else { \
            /* update array element */ \
            *dst = src->array_values_method().access_method(idx); \
        } \
    } \
)

/**
 * @brief A macro used to add setter functions for element of array of simple data types
*/
#define REGISTER_ARRAY_SETTER_AT(kind_case, cast, mutable_array_values_method, size_method, set_method) \
setterAt.addFunction(kind_case, \
    [](Value *dst, const void *srcAddr, catena::ValueIndex idx) { \
        auto *src = reinterpret_cast<const cast *>(srcAddr); \
        if (idx >= dst->mutable_array_values_method()->size_method()) { \
            /* range error */ \
            std::stringstream err; \
            err << "array index is out of bounds, " << idx \
                << " >= " << dst->mutable_array_values_method()->size_method(); \
            BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE); \
        } else { \
            /* update array element */ \
            dst->mutable_array_values_method()->set_method(idx, *src); \
        } \
    } \
)

/**
 * @brief A macro used to add getter functions accessible by clients
*/
#define REGISTER_VALUE_GETTER(kind_case, dst_method, src_method) \
valueGetter.addFunction(kind_case, [](Value* dst, const Value &src, ValueIndex idx) -> void { \
    dst->dst_method(src.src_method());\
})

/**
 * @brief A macro used to add setter functions accessible by clients
*/
#define REGISTER_VALUE_SETTER(kind_case, dst_method, src_method) \
valueSetter.addFunction(kind_case, [](Value &dst, const Value &src, ValueIndex idx) -> void { \
    dst.dst_method(src.src_method()); \
})

/**
 * @brief A macro used to add getter functions for array of simple data types accessible by clients
*/
#define REGISTER_ARRAY_VALUE_GETTER(kind_case, array_val_method, size_method, set_value_method, index_method) \
valueGetter.addFunction(kind_case, [](Value* dst, const Value &val, ValueIndex idx) -> void { \
    auto size = val.array_val_method().size_method(); \
    if (idx >= size) { \
        std::stringstream err; \
        err << "array index is out of bounds, " << idx \
            << " >= " << size; \
        BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE); \
    } \
    dst->set_value_method(val.array_val_method().index_method(idx)); \
})

/**
 * @brief A macro used to add setter functions for array of simple data types accessible by clients
*/
#define REGISTER_ARRAY_VALUE_SETTER(kind_case, array_val_method, size_method, mutable_array_val_method, mutable_method, value_method) \
valueSetter.addFunction(kind_case, [](Value &dst, const Value &src, ValueIndex idx) -> void { \
    auto size = src.array_val_method().size_method(); \
    if (idx >= size) { \
        std::stringstream err; \
        err << "array index is out of bounds, " << idx \
            << " >= " << size; \
        BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE); \
    } \
    dst.mutable_array_val_method()->mutable_method()->at(idx) = src.value_method(); \
})



void catena::initValueAccessors() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;  // so we only do this once

        // get our functories
        auto &setter = Setter::getInstance();
        auto &getter = Getter::getInstance();
        auto &setterAt = SetterAt::getInstance();
        auto &getterAt = GetterAt::getInstance();
        auto &variantGetter = VariantInfoGetter::getInstance();
        auto &valueGetter = ValueGetter::getInstance();
        auto &valueSetter = ValueSetter::getInstance();

        // register int getter
        REGISTER_GETTER(KindCase::kInt32Value, int32_value, int32_t);

        // register float getter
        REGISTER_GETTER(KindCase::kFloat32Value, float32_value, float);

        // register string getter
        REGISTER_GETTER(KindCase::kStringValue, string_value, std::string);

        // register int32 setter
        REGISTER_SETTER(KindCase::kInt32Value, set_int32_value, int32_t);

        // register float32 setter
        REGISTER_SETTER(KindCase::kFloat32Value, set_float32_value, float);

        // register string setter
        REGISTER_SETTER(KindCase::kStringValue, set_string_value, std::string);

        // register array of int getter
        REGISTER_ARRAY_GETTER(KindCase::kInt32ArrayValues, int32_t, int32_array_values, ints);

        // register array of float getter
        REGISTER_ARRAY_GETTER(KindCase::kFloat32ArrayValues, float, float32_array_values, floats);

        // register array of int setter
        REGISTER_ARRAY_SETTER(KindCase::kInt32ArrayValues, mutable_int32_array_values, clear_ints, int32_t, add_ints);

        //register array of float setter
        REGISTER_ARRAY_SETTER(KindCase::kFloat32ArrayValues, mutable_float32_array_values, clear_floats, float, add_floats);
    
        // register element of array of int getter
        REGISTER_ARRAY_GETTER_AT(KindCase::kInt32ArrayValues, int32_t, int32_array_values, ints_size, ints);

        // register element of array of float getter
        REGISTER_ARRAY_GETTER_AT(KindCase::kFloat32ArrayValues, float, float32_array_values, floats_size, floats);

        // register element of array of int setter
        REGISTER_ARRAY_SETTER_AT(KindCase::kInt32ArrayValues, int32_t, mutable_int32_array_values, ints_size, set_ints);

        // register element of array of float setter
        REGISTER_ARRAY_SETTER_AT(KindCase::kFloat32ArrayValues, float, mutable_float32_array_values, floats_size, set_floats);
        
        /** @todo string array gettter/setters */



        //register value getter for int32
        REGISTER_VALUE_GETTER(KindCase::kInt32Value, set_int32_value, int32_value);

        //register value getter for float32
        REGISTER_VALUE_GETTER(KindCase::kFloat32Value, set_float32_value, float32_value);

        //register value getter for string
        REGISTER_VALUE_GETTER(KindCase::kStringValue, set_string_value, string_value);

        // register value setter for int32
        REGISTER_VALUE_SETTER(KindCase::kInt32Value, set_int32_value, int32_value);

        // register value setter for float32
        REGISTER_VALUE_SETTER(KindCase::kFloat32Value, set_float32_value, float32_value);

        // register value setter for string
        REGISTER_VALUE_SETTER(KindCase::kStringValue, set_string_value, string_value);

        // register value getter for int32 array
        REGISTER_ARRAY_VALUE_GETTER(KindCase::kInt32ArrayValues, int32_array_values, ints_size, set_int32_value, ints);

        // register value getter for float array
        REGISTER_ARRAY_VALUE_GETTER(KindCase::kFloat32ArrayValues, float32_array_values, floats_size, set_float32_value, floats);

        // register value getter for string array
        REGISTER_ARRAY_VALUE_GETTER(KindCase::kStringArrayValues, string_array_values, strings_size, set_string_value, strings);

        // register value getter for struct array
        valueGetter.addFunction(KindCase::kStructArrayValues, [](Value* dst, const Value &val, ValueIndex idx) -> void {
            auto size = val.struct_array_values().struct_values_size();
            if (idx >= size) {
                std::stringstream err;
                err << "array index is out of bounds, " << idx
                    << " >= " << size;
                BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
            }
            *dst->mutable_struct_value() = val.struct_array_values().struct_values(idx);
        });

        // register value getter for variant array
        valueGetter.addFunction(KindCase::kStructVariantArrayValues, [](Value* dst, const Value &val, ValueIndex idx) -> void {
            auto size = val.struct_variant_array_values().struct_variants_size();
            if (idx >= size) {
                std::stringstream err;
                err << "array index is out of bounds, " << idx
                    << " >= " << size;
                BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
            }
            *dst->mutable_struct_variant_value() = val.struct_variant_array_values().struct_variants(idx);
        });

        // register value setter for int32 array
        REGISTER_ARRAY_VALUE_SETTER(KindCase::kInt32ArrayValues, int32_array_values, ints_size, 
                                    mutable_int32_array_values, mutable_ints, int32_value);

        // register value setter for float32 array
        REGISTER_ARRAY_VALUE_SETTER(KindCase::kFloat32ArrayValues, float32_array_values, floats_size, 
                                    mutable_float32_array_values, mutable_floats, float32_value);

        // register value setter for string array
        REGISTER_ARRAY_VALUE_SETTER(KindCase::kStringArrayValues, string_array_values, strings_size, 
                                    mutable_string_array_values, mutable_strings, string_value);

        // register value setter for struct array
        REGISTER_ARRAY_VALUE_SETTER(KindCase::kStructArrayValues, struct_array_values, struct_values_size, 
                                    mutable_struct_array_values, mutable_struct_values, struct_value);

        // register value setter for variant array
        REGISTER_ARRAY_VALUE_SETTER(KindCase::kStructVariantArrayValues, struct_variant_array_values, struct_variants_size, 
                                    mutable_struct_variant_array_values, mutable_struct_variants, struct_variant_value);
    }
}


template <> catena::Value::KindCase catena::getKindCase<int32_t>(TypeTag<int32_t>) {
    return catena::Value::KindCase::kInt32Value;
}

template <> catena::Value::KindCase catena::getKindCase<float>(TypeTag<float>) {
    return catena::Value::KindCase::kFloat32Value;
}

template <> catena::Value::KindCase catena::getKindCase<std::string>(TypeTag<std::string>) {
    return catena::Value::KindCase::kStringValue;
}

template <>
catena::Value::KindCase catena::getKindCase<std::vector<int32_t>>(TypeTag<std::vector<int32_t>>) {
    return catena::Value::KindCase::kInt32ArrayValues;
}

template <> catena::Value::KindCase catena::getKindCase<std::vector<float>>(TypeTag<std::vector<float>>) {
    return catena::Value::KindCase::kFloat32ArrayValues;
}

template <>
catena::Value::KindCase catena::getKindCase<std::vector<std::string>>(TypeTag<std::vector<std::string>>) {
    return catena::Value::KindCase::kStringArrayValues;
}

