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

using namespace catena::common;

EmptyValue catena::common::emptyValue;

template<>
catena::exception_with_status catena::common::toProto<EmptyValue>(catena::Value& dst, const EmptyValue* src, const IParamDescriptor& pd, const Authorizer& authz) {
    // do nothing
    return catena::exception_with_status{"", catena::StatusCode::OK};
}

template<>
bool catena::common::validFromProto<EmptyValue>(const catena::Value& src, const EmptyValue* dst, const IParamDescriptor& pd, catena::exception_with_status& rc, const Authorizer& authz) {
    // do nothing
    return true;
}

template<>
catena::exception_with_status catena::common::fromProto<EmptyValue>(const catena::Value& src, EmptyValue* dst, const IParamDescriptor& pd, const Authorizer& authz) {
    // do nothing
    return catena::exception_with_status{"", catena::StatusCode::OK};
}

template<>
catena::exception_with_status catena::common::toProto<int32_t>(catena::Value& dst, const int32_t* src, const IParamDescriptor& pd, const Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    // Must have read authorization
    if (!authz.readAuthz(pd)) {
        rc = catena::exception_with_status("Not authorized to read param " + pd.getOid(), catena::StatusCode::PERMISSION_DENIED);
    } else {
        dst.set_int32_value(*src);
    }
    return rc;
}

template<>
bool catena::common::validFromProto<int32_t>(const catena::Value& src, const int32_t* dst, const IParamDescriptor& pd, catena::exception_with_status& rc, const Authorizer& authz) {
    const IConstraint* constraint = pd.getConstraint();
    // Must have write authorization
    if (!authz.writeAuthz(pd)) {
        rc = catena::exception_with_status("Not authorized to write to param " + pd.getOid(), catena::StatusCode::PERMISSION_DENIED);
    // Must have correct type
    } else if (!src.has_int32_value()) {
        rc = catena::exception_with_status("Type mismatch between value and int " + pd.getOid(), catena::StatusCode::INVALID_ARGUMENT);
    // Must satisfy present constraint
    } else if (constraint && !constraint->isRange() && !constraint->satisfied(src)) {
        rc = catena::exception_with_status(pd.getOid() + " constraint not met", catena::StatusCode::INVALID_ARGUMENT);
    }
    return rc.status == catena::StatusCode::OK;
}

template<>
catena::exception_with_status catena::common::fromProto<int32_t>(const catena::Value& src, int32_t* dst, const IParamDescriptor& pd, const Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    if (validFromProto(src, dst, pd, rc, authz)) {
        const IConstraint* constraint = pd.getConstraint();
        if (!constraint || !constraint->isRange()) {
            *dst = src.int32_value();
        } else {
            // round src to a valid value in the range
            *dst = constraint->apply(src).int32_value();
        }
    }
    return rc;
}

template<>
catena::exception_with_status catena::common::toProto<float>(catena::Value& dst, const float* src, const IParamDescriptor& pd, const Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    // Must have read authorization
    if (!authz.readAuthz(pd)) {
        rc = catena::exception_with_status("Not authorized to read param " + pd.getOid(), catena::StatusCode::PERMISSION_DENIED);
    } else {
        dst.set_float32_value(*src);
    }
    return rc;
}

template<>
bool catena::common::validFromProto<float>(const catena::Value& src, const float* dst, const IParamDescriptor& pd, catena::exception_with_status& rc, const Authorizer& authz) {
    const IConstraint* constraint = pd.getConstraint();
    // Must have write authorization
    if (!authz.writeAuthz(pd)) {
        rc = catena::exception_with_status("Not authorized to write to param " + pd.getOid(), catena::StatusCode::PERMISSION_DENIED);
    // Must have correct type
    } else if (!src.has_float32_value()) {
        rc = catena::exception_with_status("Type mismatch between value and float " + pd.getOid(), catena::StatusCode::INVALID_ARGUMENT);
    // Must satisfy present constraint
    } else if (constraint && !constraint->isRange() && !constraint->satisfied(src)) {
        rc = catena::exception_with_status(pd.getOid() + " constraint not met", catena::StatusCode::INVALID_ARGUMENT);
    }
    return rc.status == catena::StatusCode::OK;
}

template<>
catena::exception_with_status catena::common::fromProto<float>(const catena::Value& src, float* dst, const IParamDescriptor& pd, const Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    if (validFromProto(src, dst, pd, rc, authz)) {
        const IConstraint* constraint = pd.getConstraint();
        if (!constraint || !constraint->isRange()) {
             *dst = src.float32_value();
        } else {
            // round src to a valid value in the range
            *dst = constraint->apply(src).float32_value();
        }
    }
    return rc;
}

template<>
catena::exception_with_status catena::common::toProto<std::string>(catena::Value& dst, const std::string* src, const IParamDescriptor& pd, const Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    // Must have read authorization
    if (!authz.readAuthz(pd)) {
        rc = catena::exception_with_status("Not authorized to read param " + pd.getOid(), catena::StatusCode::PERMISSION_DENIED);
    } else {
        dst.set_string_value(*src);
    }
    return rc;
}

template<>
bool catena::common::validFromProto<std::string>(const catena::Value& src, const std::string* dst, const IParamDescriptor& pd, catena::exception_with_status& rc, const Authorizer& authz) {
    const IConstraint* constraint = pd.getConstraint();
    // Must have write authorization
    if (!authz.writeAuthz(pd)) {
        rc = catena::exception_with_status("Not authorized to write to param " + pd.getOid(), catena::StatusCode::PERMISSION_DENIED);
    // Must have correct type
    } else if (!src.has_string_value()) {
        rc = catena::exception_with_status("Type mismatch between value and string " + pd.getOid(), catena::StatusCode::INVALID_ARGUMENT);
    // Must satisfy present constraint
    } else if (constraint && !constraint->satisfied(src)) {
       rc = catena::exception_with_status(pd.getOid() + " constraint not met", catena::StatusCode::INVALID_ARGUMENT);
    // Must not exceed max length
    } else if (src.string_value().size() > pd.max_length()) {
        rc = catena::exception_with_status("Param " + pd.getOid() + " exceeds maximum capacity", catena::StatusCode::OUT_OF_RANGE);
    }
    return rc.status == catena::StatusCode::OK;
}

template<>
catena::exception_with_status catena::common::fromProto<std::string>(const catena::Value& src, std::string* dst, const IParamDescriptor& pd, const Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    if (validFromProto(src, dst, pd, rc, authz)) {
        *dst = src.string_value();
    }
    return rc;
}

template<>
catena::exception_with_status catena::common::toProto<std::vector<int32_t>>(catena::Value& dst, const std::vector<int32_t>* src, const IParamDescriptor& pd, const Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    // Must have read authorization
    if (!authz.readAuthz(pd)) {
        rc = catena::exception_with_status("Not authorized to read param " + pd.getOid(), catena::StatusCode::PERMISSION_DENIED);
    } else {
        dst.clear_int32_array_values();
        catena::Int32List& int_array = *dst.mutable_int32_array_values();
        for (const int32_t& i : *src) {
            int_array.add_ints(i);
        }
    }
    return rc;
}

template<>
bool catena::common::validFromProto<std::vector<int32_t>>(const catena::Value& src, const std::vector<int32_t>* dst, const IParamDescriptor& pd, catena::exception_with_status& rc, const Authorizer& authz) {
    // Must have write authorization
    if (!authz.writeAuthz(pd)) {
        rc = catena::exception_with_status("Not authorized to write to param " + pd.getOid(), catena::StatusCode::PERMISSION_DENIED);
    // Must have correct type
    } else if (!src.has_int32_array_values()) {
        rc = catena::exception_with_status("Type mismatch between value and int array " + pd.getOid(), catena::StatusCode::INVALID_ARGUMENT);
    // Must not exceed max length
    } else if (src.int32_array_values().ints_size() > pd.max_length()) {
        rc = catena::exception_with_status("Param " + pd.getOid() + " exceeds maximum capacity", catena::StatusCode::OUT_OF_RANGE);
    } else {
        const IConstraint* constraint = pd.getConstraint();
        // All values must satisfy present constraint
        if (constraint && !constraint->isRange()) {
            for (int32_t i : src.int32_array_values().ints()) {
                catena::Value item;
                item.set_int32_value(i);
                if (!constraint->satisfied(item)) {
                    rc = catena::exception_with_status(pd.getOid() + " constraint not met", catena::StatusCode::INVALID_ARGUMENT);
                    break;
                }
            }
        }
    }
    return rc.status == catena::StatusCode::OK;
}

template<>
catena::exception_with_status catena::common::fromProto<std::vector<int32_t>>(const Value& src, std::vector<int32_t>* dst, const IParamDescriptor& pd, const Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    if (validFromProto(src, dst, pd, rc, authz)) {
        dst->clear();
        const IConstraint* constraint = pd.getConstraint();
        for (int32_t i : src.int32_array_values().ints()) {
            catena::Value item;
            item.set_int32_value(i);
            if (!constraint || !constraint->isRange()) {
                dst->push_back(i);
            } else {
                // round src to a valid value in the range
                dst->push_back(constraint->apply(item).int32_value());
            }
        }
    }
    return rc;
}

template<>
catena::exception_with_status catena::common::toProto<std::vector<float>>(catena::Value& dst, const std::vector<float>* src, const IParamDescriptor& pd, const Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    // Must have read authorization
    if (!authz.readAuthz(pd)) {
        rc = catena::exception_with_status("Not authorized to read param " + pd.getOid(), catena::StatusCode::PERMISSION_DENIED);
    } else {
        dst.clear_float32_array_values();
        catena::Float32List& float_array = *dst.mutable_float32_array_values();
        for (const float& f : *src) {
            float_array.add_floats(f);
        }
    }
    return rc;
}

template<>
bool catena::common::validFromProto<std::vector<float>>(const catena::Value& src, const std::vector<float>* dst, const IParamDescriptor& pd, catena::exception_with_status& rc, const Authorizer& authz) {
    // Must have write authorization
    if (!authz.writeAuthz(pd)) {
        rc = catena::exception_with_status("Not authorized to write to param " + pd.getOid(), catena::StatusCode::PERMISSION_DENIED);
    // Must have correct type
    } else if (!src.has_float32_array_values()) {
        rc = catena::exception_with_status("Type mismatch between value and float array " + pd.getOid(), catena::StatusCode::INVALID_ARGUMENT);
    // Must not exceed max length
    } else if (src.float32_array_values().floats_size() > pd.max_length()) {
        rc = catena::exception_with_status("Param " + pd.getOid() + " exceeds maximum capacity", catena::StatusCode::OUT_OF_RANGE);
    } else {
        const IConstraint* constraint = pd.getConstraint();
        // All values must satisfy present constraint
        if (constraint && !constraint->isRange()) {
            for (float i : src.float32_array_values().floats()) {
                catena::Value item;
                item.set_float32_value(i);
                if (!constraint->satisfied(item)) {
                    rc = catena::exception_with_status(pd.getOid() + " constraint not met", catena::StatusCode::INVALID_ARGUMENT);
                    break;
                }
            }
        }
    }
    return rc.status == catena::StatusCode::OK;
}

template<>
catena::exception_with_status catena::common::fromProto<std::vector<float>>(const Value& src, std::vector<float>* dst, const IParamDescriptor& pd, const Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    if (validFromProto(src, dst, pd, rc, authz)) {
        dst->clear();
        const IConstraint* constraint = pd.getConstraint();
        for (float f : src.float32_array_values().floats()) {
            catena::Value item;
            item.set_float32_value(f);
            if (!constraint || !constraint->isRange()) {
                dst->push_back(f);
            } else {
                // round src to a valid value in the range
                dst->push_back(constraint->apply(item).float32_value());
            }
        }
    }
    return rc;
}

template<>
catena::exception_with_status catena::common::toProto<std::vector<std::string>>(catena::Value& dst, const std::vector<std::string>* src, const IParamDescriptor& pd, const Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    // Must have read authorization
    if (!authz.readAuthz(pd)) {
        rc = catena::exception_with_status("Not authorized to read param " + pd.getOid(), catena::StatusCode::PERMISSION_DENIED);
    } else {
        dst.clear_string_array_values();
        catena::StringList& string_array = *dst.mutable_string_array_values();
        for (const std::string& s :*src) {
            string_array.add_strings(s);
        }
    }
    return rc;
}

template<>
bool catena::common::validFromProto<std::vector<std::string>>(const catena::Value& src, const std::vector<std::string>* dst, const IParamDescriptor& pd, catena::exception_with_status& rc, const Authorizer& authz) {
    std::size_t total_length = 0;
    // Must have write authorization
    if (!authz.writeAuthz(pd)) {
        rc = catena::exception_with_status("Not authorized to write to param " + pd.getOid(), catena::StatusCode::PERMISSION_DENIED);
    // Must have correct type
    } else if (!src.has_string_array_values()) {
        rc = catena::exception_with_status("Type mismatch between value and string array " + pd.getOid(), catena::StatusCode::INVALID_ARGUMENT);
    // Must not exceed max length
    } else if (src.string_array_values().strings_size() > pd.max_length()) {
        rc = catena::exception_with_status("Param " + pd.getOid() + " exceeds maximum capacity", catena::StatusCode::OUT_OF_RANGE);
    } else {
        const IConstraint* constraint = pd.getConstraint();
        for (const std::string& s : src.string_array_values().strings()) {
            total_length += s.length();
            // All values must satisfy present constraint
            if (constraint) {
                catena::Value item;
                item.set_string_value(s);
                if (!constraint->satisfied(item)) {
                    rc = catena::exception_with_status(pd.getOid() + " constraint not met", catena::StatusCode::INVALID_ARGUMENT);
                    break;
                }
            }
        }
    }
    // Sum of strings must not exceed total length
    if (total_length > pd.total_length()) {
        rc = catena::exception_with_status("String array param " + pd.getOid() + " exceeds total length", catena::StatusCode::OUT_OF_RANGE);
    }
    return rc.status == catena::StatusCode::OK;
}

template<>
catena::exception_with_status catena::common::fromProto<std::vector<std::string>>(const Value& src, std::vector<std::string>* dst, const IParamDescriptor& pd, const Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    if (validFromProto(src, dst, pd, rc, authz)) {
        dst->clear();
        for (const std::string& s : src.string_array_values().strings()) {
            dst->push_back(s);
        }
    }
    return rc;
}
