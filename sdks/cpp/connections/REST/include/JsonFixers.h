#pragma once

/*
 * Copyright 2026 Ross Video Ltd
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file JsonFixers.h
 * @brief Reusable picojson post-processing primitives and the per-message
 * "fix" compositions used by RestJsonFormatter.
 *
 * These mirror the Go SDK's transports/json_utils.go cleanup pass: protobuf
 * treats default values as unset, but the st2138 schema sometimes requires a
 * default-valued field to be present (e.g. Device.slot == 0) and sometimes
 * requires an empty field to be omitted (e.g. a non-struct Param's empty
 * params map). A single JsonPrintOptions cannot express both, so we serialize
 * and then surgically fix the JSON.
 *
 * @copyright Copyright © 2026 Ross Video Ltd
 */

// protobuf
#include <google/protobuf/message.h>

// picojson
//
// NOTE: We deliberately do NOT define PICOJSON_USE_INT64 here. picojson's
// get<double>() transparently reads numbers stored as int64 (jwt-cpp enables
// that mode elsewhere in the program), so the double accessors interoperate
// seamlessly and we never need an int64 code path of our own.
#include <picojson/picojson.h>

// std
#include <set>
#include <string>

namespace catena {
namespace REST {

/**
 * @brief Recursively removes object members whose values are "empty" (null,
 * "", {}, or []), cascading so that a parent which becomes empty is itself
 * removed by its caller. Array elements are cleaned in place but never removed
 * (to avoid shifting indices), matching the Go reference behaviour.
 * @param v The parsed JSON tree to clean in place.
 * @return true if @p v is itself considered empty after cleaning.
 */
bool stripEmptyValues(picojson::value& v);

/**
 * @brief Recursively removes object members named in @p fields whose value is
 * the number 0. Used for schema-forbidden zero defaults such as "precision",
 * "max_length" and "total_length".
 * @param v The parsed JSON tree to clean in place.
 * @param fields The set of field names to strip when zero.
 */
void stripZeroFields(picojson::value& v, const std::set<std::string>& fields);

/**
 * @brief Removes "response": false everywhere inside the "params" subtree of
 * @p v, leaving the "commands" subtree untouched (response is only valid on
 * commands).
 * @param v The parsed JSON tree to clean in place.
 */
void stripResponseFalseUnderParams(picojson::value& v);

/**
 * @brief Sets @p key to @p val on the root object only if it is not already
 * present. No-op if @p root is not an object.
 * @param root The parsed JSON object to modify in place.
 * @param key The member name to inject.
 * @param val The value to inject when @p key is absent.
 */
void injectIfAbsent(picojson::value& root, const std::string& key, const picojson::value& val);

/**
 * @brief PostFn for st2138::Device: strips schema-forbidden zero fields, the
 * "response" flag from params, and all empty values.
 * @param msg The originating message (unused; signature matches PostFn).
 * @param json The serialized JSON to fix in place.
 */
void fixDevice(const google::protobuf::Message& msg, std::string& json);

/**
 * @brief PostFn for st2138::PushUpdates: ensures "slot" is present even when it
 * is the proto3 default of 0.
 * @param msg The originating PushUpdates message (read for its slot value).
 * @param json The serialized JSON to fix in place.
 */
void fixPushUpdates(const google::protobuf::Message& msg, std::string& json);

}  // namespace REST
}  // namespace catena
