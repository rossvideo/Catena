#pragma once

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

/**
 * @file NamedChoiceConstraint.h
 * @brief A constraint that checks if a value is within some named choices
 * @author isaac.robert@rossvideo.com
 * @date 2024-07-25
 * @copyright Copyright (c) 2024 Ross Video
 */

// commmon
#include <IConstraint.h>
#include <IParam.h>
#include <Tags.h>

// lite
#include <PolyglotText.h>
#include <Device.h>

#include <google/protobuf/message_lite.h>

#include <string>
#include <unordered_set>
#include <initializer_list>

namespace catena {
namespace lite {

/**
 * @brief Named choice constraint, ensures a value is within a named choice
 * @tparam T int or string
 */
template <typename T>
class NamedChoiceConstraint : public catena::common::IConstraint {
public:
    /**
     * @brief map of choices with their display names
     */
    using Choices = std::unordered_map<T, PolyglotText>;
    /**
     * @brief initializer list for choices
     */
    using ListInitializer = std::initializer_list<std::pair<T, PolyglotText::ListInitializer>>;

public:
    /**
     * @brief Construct a new Named Choice Constraint object
     * @param init the list of choices
     * @param strict should the value be constrained if not in choices
     * @param oid the oid of the constraint
     * @param shared is the constraint shared
     * @param dm the device to add the constraint to
     * @note  the first choice provided will be the default for the constraint
     */
    NamedChoiceConstraint(ListInitializer init, bool strict, std::string oid, bool shared, Device& dm)
        : choices_{init.begin(), init.end()}, 
        strict_{strict}, default_{init.begin()->first} {
        dm.addItem<common::ConstraintTag>(oid, this);
    }
    
    /**
     * @brief Construct a new Named Choice Constraint object
     * @param init the list of choices
     * @param strict should the value be constrained if not in choices
     * @param oid the oid of the constraint
     * @param shared is the constraint shared
     * @param parent the param to add the constraint to
     * @note  the first choice provided will be the default for the constraint
     */
    NamedChoiceConstraint(ListInitializer init, bool strict, std::string oid, bool shared)
        : choices_{init.begin(), init.end()}, 
        strict_{strict}, default_{init.begin()->first} {}

    /**
     * @brief default destructor
     */
    virtual ~NamedChoiceConstraint() = default;

    /**
     * @brief checks if the value satisfies the constraint
     * @param src the value to check
     * @return true if the value satisfies the constraint
     */
    bool satisfied(const catena::Value& src) const override {

        if constexpr(std::is_same<T, int32_t>::value) {
            return choices_.find(src.int32_value()) != choices_.end();
        }

        if constexpr(std::is_same<T, std::string>::value) {
            if (!strict_) {
                return true;
            }
            return choices_.find(src.string_value()) != choices_.end();
        }
    }

    /**
     * If a request does not satisfy a choice constraint, then
     * the request is invalid and should be ignored.
     * 
     * Calling this will always return an empty value.
     */
    catena::Value apply(const catena::Value& src) const override {
        catena::Value val;
        return val;
    }

    /**
     * @brief serialize the constraint to a protobuf message
     * @param msg the protobuf message to populate
     */
    void toProto(catena::Constraint& constraint) const override {

        if constexpr(std::is_same<T, int32_t>::value) {
            constraint.set_type(catena::Constraint::INT_CHOICE);
            for (auto& [value, name] : choices_) {
                auto intChoice = constraint.mutable_int32_choice()->add_choices();
                intChoice->set_value(value);
                name.toProto(*intChoice->mutable_name());
            }
        }

        if constexpr(std::is_same<T, std::string>::value) {
            constraint.set_type(catena::Constraint::STRING_STRING_CHOICE);
            for (auto& [value, name] : choices_) {
                auto stringChoice = constraint.mutable_string_string_choice()->add_choices();
                stringChoice->set_value(value);
                name.toProto(*stringChoice->mutable_name());
            }
        }
    }

    bool isRange() const override { return false; }

private:
    Choices choices_; ///< the choices
    bool strict_;     ///< should the value be constrained on apply
    T default_;       ///< the default value to constrain to
};

} // namespace lite
} // namespace catena