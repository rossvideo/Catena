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

// lite
#include <PicklistConstraint.h>

// protobuf interface
#include <interface/param.pb.h>

using IParam = catena::common::IParam;

namespace catena {
namespace lite {

PicklistConstraint::PicklistConstraint(ListInitializer init, bool strict, std::string oid, 
    bool shared, Device& dm)
    : IConstraint{oid, shared}, choices_{init.begin(), init.end()}, 
    strict_{strict}, default_{*init.begin()} {
    dm.addItem<common::ConstraintTag>(oid, this);
}

PicklistConstraint::PicklistConstraint(ListInitializer init, bool strict, std::string oid, 
    bool shared, IParam* parent)
    : IConstraint{oid, shared}, choices_{init.begin(), init.end()}, 
    strict_{strict}, default_{*init.begin()} {
    parent->setConstraint(this);
}

PicklistConstraint::~PicklistConstraint() = default;

void PicklistConstraint::apply(void* src) const {
    auto& src_val = *reinterpret_cast<catena::Value*>(src);

    // ignore the request if src is not valid
    if (!src_val.has_string_value()) { return; }

    // constrain if strict and src is not in choices
    if (strict_ && !choices_.contains(src_val.string_value())) {
        src_val.set_string_value(default_);
    }
}

void PicklistConstraint::toProto(google::protobuf::MessageLite& msg) const {
    auto& constraint = dynamic_cast<catena::Constraint&>(msg);

    constraint.set_type(catena::Constraint::STRING_STRING_CHOICE);
    for (std::string choice : choices_) {
        constraint.mutable_string_choice()->add_choices(choice);
    }
}

} // namespace lite
} // namespace catena