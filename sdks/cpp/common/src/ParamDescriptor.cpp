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


#include <ParamDescriptor.h>
#include <Authorization.h>  
#include <Device.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_format.h"

#define DEFAULT_MAX_LENGTH 1024
ABSL_FLAG(int, param_max_length, DEFAULT_MAX_LENGTH, "default maximum length for parameter's with max_length undefined.");

using catena::common::ParamDescriptor;

const int ParamDescriptor::max_length() {
    if (max_length_ <= 0) {
        int default_max = absl::GetFlag(FLAGS_param_max_length);
        max_length_ = default_max <= 0 ? DEFAULT_MAX_LENGTH : default_max;
    }
    return max_length_;
}

void ParamDescriptor::toProto(catena::Param &param, Authorizer& authz) const {
    param.set_type(type_);
    for (const auto& oid_alias : oid_aliases_) {
        param.add_oid_aliases(oid_alias);
    }
    for (const auto& [lang, text] : name_.displayStrings()) {
        (*param.mutable_name()->mutable_display_strings())[lang] = text;
    }
    param.set_widget(widget_);
    param.set_read_only(read_only_);
    if (constraint_) {
        if (constraint_->isShared()) {
            *param.mutable_constraint()->mutable_ref_oid() = constraint_->getOid();
        } else {
            constraint_->toProto(*param.mutable_constraint());
        }
    }

    auto* dstParams = param.mutable_params();
    for (const auto& [oid, subParam] : subParams_) {
        if (authz.readAuthz(*subParam)) {
            subParam->toProto((*dstParams)[oid], authz);
        }
    }
}

const std::string& ParamDescriptor::name(const std::string& language) const { 
    auto it = name_.displayStrings().find(language);
    if (it != name_.displayStrings().end()) {
        return it->second;
    } else {
        static const std::string empty;
        return empty;
    }
}

const std::string& ParamDescriptor::getScope() const {
    if (!scope_.empty()) {
        return scope_;
    } else if (parent_) {
        return parent_->getScope();
    } else {
        return dev_.get().getDefaultScope();
    }
}