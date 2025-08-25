/*
 * Copyright 2025 Ross Video Ltd
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

#include <ChoiceConstraint.h>

using catena::common::ChoiceConstraint;

// INT_CHOICE implementation
template<>
bool ChoiceConstraint<int32_t, st2138::Constraint::INT_CHOICE>::satisfied(const st2138::Value& src) const {
    return choices_.contains(src.int32_value());
}

template<>
void ChoiceConstraint<int32_t, st2138::Constraint::INT_CHOICE>::toProto(st2138::Constraint& constraint) const {
    constraint.set_type(st2138::Constraint::INT_CHOICE);
    for (auto& [value, name] : choices_) {
        auto intChoice = constraint.mutable_int32_choice()->add_choices();
        intChoice->set_value(value);
        name.toProto(*intChoice->mutable_name());
    }
}

// STRING_CHOICE implementation
template<>
bool ChoiceConstraint<std::string, st2138::Constraint::STRING_CHOICE>::satisfied(const st2138::Value& src) const {
    return !strict_ || choices_.contains(src.string_value());
}

template<>
void ChoiceConstraint<std::string, st2138::Constraint::STRING_CHOICE>::toProto(st2138::Constraint& constraint) const {
    constraint.set_type(st2138::Constraint::STRING_CHOICE);
    for (auto& [value, name] : choices_) {
        *constraint.mutable_string_choice()->add_choices() = value;
    }
}

// STRING_STRING_CHOICE implementation
template<>
bool ChoiceConstraint<std::string, st2138::Constraint::STRING_STRING_CHOICE>::satisfied(const st2138::Value& src) const {
    return !strict_ || choices_.contains(src.string_value());
}

template<>
void ChoiceConstraint<std::string, st2138::Constraint::STRING_STRING_CHOICE>::toProto(st2138::Constraint& constraint) const {
    constraint.set_type(st2138::Constraint::STRING_STRING_CHOICE);
    for (auto& [value, name] : choices_) {
        auto stringChoice = constraint.mutable_string_string_choice()->add_choices();
        stringChoice->set_value(value);
        name.toProto(*stringChoice->mutable_name());
    }
}
