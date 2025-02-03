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

//common
#include <IParam.h>
#include <StructInfo.h>

// protobuf interface
#include <interface/param.pb.h>

using catena::common::Authorizer;
using catena::common::EmptyValue;
using ::catena::Value;

EmptyValue catena::common::emptyValue;

template<>
void catena::common::toProto<EmptyValue>(::catena::Value& dst, const EmptyValue* src, const ParamDescriptor& pd, const Authorizer& authz) {
    // do nothing
}

template<>
void catena::common::fromProto<EmptyValue>(const catena::Value& src, EmptyValue* dst, const ParamDescriptor& pd, const Authorizer& authz) {
    // do nothing
}

// ----------------------------------- INT -----------------------------------
template<>
void catena::common::toProto<int32_t>(Value& dst, const int32_t* src, const ParamDescriptor& pd, const Authorizer& authz) {
    dst.set_int32_value(*src);
}

template<>
void catena::common::fromProto<int32_t>(const catena::Value& src, int32_t* dst, const ParamDescriptor& pd, const Authorizer& authz) {
    const catena::common::IConstraint* constraint = pd.getConstraint();

    if (!src.has_int32_value()) {
        return;
    }
    if (!constraint || constraint->satisfied(src)) {
        *dst = src.int32_value();
    } else  if (constraint->isRange()) {
        // round src to a valid value in the range
        *dst = constraint->apply(src).int32_value();
    } 
    // if the constraint is not satified and not a range, then the value is unchanged
    return;
}
// ---------------------------------- FLOAT ----------------------------------
template<>
void catena::common::toProto<float>(catena::Value& dst, const float* src, const ParamDescriptor& pd, const Authorizer& authz) {
    dst.set_float32_value(*src);
}

template<>
void catena::common::fromProto<float>(const catena::Value& src, float* dst, const ParamDescriptor& pd, const Authorizer& authz) {
    const catena::common::IConstraint* constraint = pd.getConstraint();

    if (!src.has_float32_value()) {
        return;
    }
    if (!constraint || constraint->satisfied(src)) {
        *dst = src.float32_value();
    } else {
        // float32 constraint is always a range
        // round src to a valid value in the range
        *dst = constraint->apply(src).float32_value();
    }
}
// ---------------------------------- STRING ----------------------------------
template<>
void catena::common::toProto<std::string>(Value& dst, const std::string* src, const ParamDescriptor& pd, const Authorizer& authz) {
    dst.set_string_value(*src);
}

template<>
void catena::common::fromProto<std::string>(const catena::Value& src, std::string* dst, const ParamDescriptor& pd, const Authorizer& authz) {
    const catena::common::IConstraint* constraint = pd.getConstraint();

    if (!src.has_string_value()) {
        return;
    }

    // if the parameter does not satisfy the constraint, then the element is unchanged
    if (!constraint || constraint->satisfied(src)) {
        *dst = src.string_value();
    }
    return;
}
// -------------------------------- INT ARRAY --------------------------------
template<>
void catena::common::toProto<std::vector<int32_t>>(Value& dst, const std::vector<int32_t>* src, const ParamDescriptor& pd, const Authorizer& authz) {
    dst.clear_int32_array_values();
    catena::Int32List& int_array = *dst.mutable_int32_array_values();
    for (const int32_t& i : *src) {
        int_array.add_ints(i);
    }
}

template<>
void catena::common::fromProto<std::vector<int32_t>>(const Value& src, std::vector<int32_t>* dst, const ParamDescriptor& pd, const Authorizer& authz) {
    if (!src.has_int32_array_values()) {
        return;
    }
    const catena::Int32List& int_array = src.int32_array_values();
    const catena::common::IConstraint* constraint = pd.getConstraint();
    catena::Value item;
    
    // Right now from proto is able to append any number of values to the vector
    // Do we want to keep this behavior?
    for (int i = 0; i < int_array.ints_size(); ++i) {
        item.set_int32_value(int_array.ints(i));
        if (!constraint || constraint->satisfied(item)) {
            if (i < dst->size()) {
                (*dst)[i] = int_array.ints(i);
            } else {
                dst->push_back(int_array.ints(i));
            }
        } else if (constraint->isRange()) {
            // round src to a valid value in the range
            if (i < dst->size()) {
                (*dst)[i] = constraint->apply(item).int32_value();
            } else {
                dst->push_back(constraint->apply(item).int32_value());
            }
        }
    }
    return;
}
// ------------------------------- FLOAT ARRAY -------------------------------
template<>
void catena::common::toProto<std::vector<float>>(Value& dst, const std::vector<float>* src, const ParamDescriptor& pd, const Authorizer& authz) {
    dst.clear_float32_array_values();
    catena::Float32List& float_array = *dst.mutable_float32_array_values();
    for (const float& f : *src) {
        float_array.add_floats(f);
    }
}

template<>
void catena::common::fromProto<std::vector<float>>(const Value& src, std::vector<float>* dst, const ParamDescriptor& pd, const Authorizer& authz) {
    if (!src.has_float32_array_values()) {
        return;
    }
    dst->clear();
    const catena::Float32List& float_array = src.float32_array_values();
    const catena::common::IConstraint* constraint = pd.getConstraint();
    catena::Value item;

    // Right now from proto is able to append any number of values to the vector
    // Do we want to keep this behavior?
    for (int i = 0; i < float_array.floats_size(); ++i) {
        item.set_float32_value(float_array.floats(i));
        if (!constraint || constraint->satisfied(item)) {
            dst->push_back(float_array.floats(i));
        } else {
            // float32 constraint is always a range
            // round src to a valid value in the range
            dst->push_back(constraint->apply(item).float32_value());
        }
    }
    return;
}
// ------------------------------- STRING ARRAY -------------------------------
template<>
void catena::common::toProto<std::vector<std::string>>(Value& dst, const std::vector<std::string>* src, const ParamDescriptor& pd, const Authorizer& authz) {
    dst.clear_string_array_values();
    catena::StringList& string_array = *dst.mutable_string_array_values();
    for (const std::string& s :*src) {
        string_array.add_strings(s);
    }
}

template<>
void catena::common::fromProto<std::vector<std::string>>(const Value& src, std::vector<std::string>* dst, const ParamDescriptor& pd, const Authorizer& authz) {
    if (!src.has_string_array_values()) {
        return;
    }
    dst->clear();
    const catena::StringList& string_array = src.string_array_values();
    const catena::common::IConstraint* constraint = pd.getConstraint();
    catena::Value item;

    for (int i = 0; i < string_array.strings_size(); ++i) {
        item.set_string_value(string_array.strings(i));
        // if parameter does not satisfy the constraint, then the element is unchanged
        if (!constraint || constraint->satisfied(item)) {
            dst->push_back(string_array.strings(i));
        }
            
    }
    return;
}

