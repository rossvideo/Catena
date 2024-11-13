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

// common
#include <PicklistConstraint.h>

// protobuf interface
#include <interface/param.pb.h>

using IParam = catena::common::IParam;

using catena::common::PicklistConstraint;

PicklistConstraint::PicklistConstraint(ListInitializer init, bool strict, std::string oid, 
    bool shared, Device& dm)
    : choices_{init.begin(), init.end()}, 
    strict_{strict}, oid_{oid}, default_{*init.begin()}, shared_{shared} {
    dm.addItem<common::ConstraintTag>(oid, this);
}

PicklistConstraint::PicklistConstraint(ListInitializer init, bool strict, std::string oid, 
    bool shared)
    : choices_{init.begin(), init.end()}, 
    strict_{strict}, oid_{oid}, default_{*init.begin()}, shared_{shared} {}

PicklistConstraint::~PicklistConstraint() = default;

bool PicklistConstraint::satisfied(const catena::Value& src) const {
    if (!strict_) {
        return true;
    }

    return choices_.find(src.string_value()) != choices_.end();
}

/**
 * Named choice constraint can't be applied. Calling this
 * will always return an empty value.
 */
catena::Value PicklistConstraint::apply(const catena::Value& src) const {
    catena::Value val;
    return val;
}

void PicklistConstraint::toProto(catena::Constraint& constraint) const {

    constraint.set_type(catena::Constraint::STRING_CHOICE);
    for (std::string choice : choices_) {
        constraint.mutable_string_choice()->add_choices(choice);
    }
}
