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


using catena::common::ParamDescriptor;

uint32_t ParamDescriptor::max_length() const {
    return (max_length_ > 0) ? max_length_ : dev_.get().default_max_length();
}

std::size_t ParamDescriptor::total_length() const {
    return (total_length_ > 0) ? total_length_ : dev_.get().default_total_length();
}

void ParamDescriptor::toProto(catena::Param &param, Authorizer& authz) const {
    
    param.set_type(type_);
    param.set_read_only(read_only_);
    param.set_widget(widget_);
    param.set_minimal_set(minimal_set_);

    for (const auto& oid_alias : oid_aliases_) {
        param.add_oid_aliases(oid_alias);
    }
    for (const auto& [lang, text] : name_.displayStrings()) {
        (*param.mutable_name()->mutable_display_strings())[lang] = text;
    }
    
    
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

    param.set_template_oid(template_oid_);
}


void ParamDescriptor::toProto(catena::BasicParamInfo &paramInfo, Authorizer& authz) const {
    paramInfo.set_type(type_);
    paramInfo.set_oid(oid_);
    paramInfo.set_template_oid(template_oid_);

    for (const auto& [lang, text] : name_.displayStrings()) {
        (*paramInfo.mutable_name()->mutable_display_strings())[lang] = text;
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