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

#include <JsonFixers.h>

// proto
#include <interface/device.pb.h>

namespace catena {
namespace REST {

namespace {

/*
 * True if val holds a JSON number equal to zero. picojson's is<double>() is true
 * for both number- and int64-stored values, and get<double>() reads either,
 * so this works regardless of how the number was stored.
 */
bool isZeroNumber(picojson::value& val) {
    return val.is<double>() && val.get<double>() == 0.0;
}

/*
 * Recursively deletes "response": false from an object tree. Mirrors the Go
 * reference deleteResponseFalse: only object-valued members are recursed into.
 */
void deleteResponseFalse(picojson::value& val) {
    if (!val.is<picojson::object>()) { return; }
    auto& obj = val.get<picojson::object>();
    bool needsDelete = false;
    for (auto& kv : obj) {
        if (kv.first == "response" && kv.second.is<bool>() && !kv.second.get<bool>()) {
            needsDelete = true;
        } else if (kv.second.is<picojson::object>()) {
            deleteResponseFalse(kv.second);
        }
    }
    if (needsDelete) { obj.erase("response"); }
}

/*
 * True if val should be deleted by stripEmptyValues (null, "", or a nested
 * object/array that is empty after its own cleaning).
 */
bool shouldDeleteValue(picojson::value& val) {
    if (val.is<picojson::null>()) { return true; }
    if (val.is<std::string>()) { return val.get<std::string>().empty(); }
    if (val.is<picojson::object>() || val.is<picojson::array>()) { return stripEmptyValues(val); }
    return false;
}

}  // namespace

void stripZeroFields(picojson::value& val, const std::set<std::string>& fields) {
    if (val.is<picojson::array>()) {
        for (auto& e : val.get<picojson::array>()) { stripZeroFields(e, fields); }
        return;
    }
    if (!val.is<picojson::object>()) { return; }
    auto& obj = val.get<picojson::object>();
    for (auto it = obj.begin(); it != obj.end();) {
        picojson::value& val = it->second;
        if (val.is<picojson::object>() || val.is<picojson::array>()) {
            stripZeroFields(val, fields);
            ++it;
        } else if (fields.count(it->first) != 0 && isZeroNumber(val)) {
            it = obj.erase(it);
        } else {
            ++it;
        }
    }
}

void stripResponseFalseUnderParams(picojson::value& val) {
    if (!val.is<picojson::object>()) { return; }
    auto& obj = val.get<picojson::object>();
    auto it = obj.find("params");
    if (it == obj.end() || !it->second.is<picojson::object>()) { return; }
    deleteResponseFalse(it->second);
}

bool stripEmptyValues(picojson::value& val) {
    if (val.is<picojson::object>()) {
        auto& obj = val.get<picojson::object>();
        for (auto it = obj.begin(); it != obj.end();) {
            if (shouldDeleteValue(it->second)) {
                it = obj.erase(it);
            } else {
                ++it;
            }
        }
        return obj.empty();
    }
    if (val.is<picojson::array>()) {
        auto& arr = val.get<picojson::array>();
        for (auto& e : arr) { stripEmptyValues(e); }
        return arr.empty();
    }
    return false;
}

void injectIfAbsent(picojson::value& root, const std::string& key, const picojson::value& val) {
    if (!root.is<picojson::object>()) { return; }
    auto& obj = root.get<picojson::object>();
    if (obj.find(key) == obj.end()) { obj[key] = val; }
}

// post process Device messages
void fixDevice(const google::protobuf::Message& /*msg*/, std::string& json) {
    picojson::value root;
    const std::string err = picojson::parse(root, json);
    if (!err.empty()) { return; }  // GCOVR_EXCL_LINE - input is always our own serialization
    stripZeroFields(root, {"precision", "max_length", "total_length"});
    stripResponseFalseUnderParams(root);
    stripEmptyValues(root);
    json = root.serialize();
}

// post process PushUpdates messages
void fixPushUpdates(const google::protobuf::Message& msg, std::string& json) {
    const auto* pu = dynamic_cast<const st2138::PushUpdates*>(&msg);
    if (pu == nullptr) { return; }  // GCOVR_EXCL_LINE - only registered for PushUpdates
    picojson::value root;
    const std::string err = picojson::parse(root, json);
    if (!err.empty()) { return; }  // GCOVR_EXCL_LINE - input is always our own serialization
    injectIfAbsent(root, "slot", picojson::value(static_cast<double>(pu->slot())));
    json = root.serialize();
}

}  // namespace REST
}  // namespace catena
