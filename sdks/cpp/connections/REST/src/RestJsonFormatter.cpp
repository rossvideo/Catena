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

#include <RestJsonFormatter.h>
#include <JsonFixers.h>

// proto
#include <interface/device.pb.h>

namespace catena {
namespace REST {

RestJsonFormatter::RestJsonFormatter(Protector) {
    // Device: emit proto3 defaults (so slot:0 etc. are present) then strip the
    // fields the st2138 schema forbids when empty/zero.
    {
        Rule r;
        r.options.always_print_fields_with_no_presence = true;
        r.post = &fixDevice;
        rules_[st2138::Device::descriptor()] = std::move(r);
    }
    // PushUpdates: sparse serialization, then re-inject slot if it was the
    // proto3 default of 0 (slot must always be present).
    {
        Rule r;
        r.post = &fixPushUpdates;
        rules_[st2138::PushUpdates::descriptor()] = std::move(r);
    }
}

catena::exception_with_status RestJsonFormatter::format(const google::protobuf::Message& msg,
                                                        std::string& out) const {
    out.clear();

    google::protobuf::util::JsonPrintOptions options;
    const Rule* rule = nullptr;
    auto it = rules_.find(msg.GetDescriptor());
    if (it != rules_.end()) {
        options = it->second.options;
        rule = &it->second;
    }
    // The st2138 spec requires snake_case field names; force it regardless of
    // any per-rule options.
    options.preserve_proto_field_names = true;

    auto status = google::protobuf::util::MessageToJsonString(msg, &out, options);
    if (!status.ok()) {  // GCOVR_EXCL_START
        return catena::exception_with_status(std::string(status.message()),
                                             catena::StatusCode::INVALID_ARGUMENT);
    }  // GCOVR_EXCL_STOP

    if (rule != nullptr && rule->post) { rule->post(msg, out); }

    return catena::exception_with_status("", catena::StatusCode::OK);
}

void RestJsonFormatter::setRule(const google::protobuf::Descriptor* desc, Rule rule) {
    rules_[desc] = std::move(rule);
}

bool RestJsonFormatter::hasRule(const google::protobuf::Descriptor* desc) const {
    return rules_.find(desc) != rules_.end();
}

RestJsonFormatter::RuleTable RestJsonFormatter::snapshot() const {
    return rules_;
}

void RestJsonFormatter::restore(RuleTable rules) {
    rules_ = std::move(rules);
}

}  // namespace REST
}  // namespace catena
