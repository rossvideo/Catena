#pragma once

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

/**
 * @file ChoiceConstraint.h
 * @brief A constraint that checks if a value is within some named choices
 * @author isaac.robert@rossvideo.com
 * @author benjamin.whitten@rossvideo.com
 * @date 2025-08-19
 * @copyright Copyright (c) 2024 Ross Video
 */

/**
 * @example use_constraints.cpp
 * Demonstrates use of this and other constraints.
 */

// commmon
#include <IConstraint.h>
#include <Tags.h>
#include <PolyglotText.h>
#include <IDevice.h>

// std
#include <string>
#include <unordered_set>
#include <initializer_list>
#include <type_traits>

namespace catena {
namespace common {

/**
 * @brief Named choice constraint which ensures a value is within a named choice.
 * 
 * Supports three types: INT_CHOICE, STRING_CHOICE, and STRING_STRING_CHOICE.
 * 
 * INT_CHOICE and STRING_STRING_CHOICE have both a value and a polyglot text
 * "name" for each choice while STRING_CHOICE is just a value.
 * 
 * @tparam T The constraint's primitive type. Must be int or string.
 * @tparam cType The constraint's named type, must be one of INT_CHOICE,
 * STRING_CHOICE, or STRING_STRING_CHOICE.
 */
template <typename T, st2138::Constraint::ConstraintType cType>
class ChoiceConstraint : public catena::common::IConstraint {
  public:
    static_assert(
        (std::is_same<T, int32_t>::value && cType == st2138::Constraint::INT_CHOICE) ||
        (std::is_same<T, std::string>::value && cType == st2138::Constraint::STRING_CHOICE) ||
        (std::is_same<T, std::string>::value && cType == st2138::Constraint::STRING_STRING_CHOICE),
        "ChoiceConstraint: Invalid combination of T and constraintType"
    );

    /**
     * @brief map of choices with their display names
     */
    using Choices = std::unordered_map<T, PolyglotText>;
    /**
     * @brief initializer list for choices
     */
    using ListInitializer = std::initializer_list<std::pair<T, PolyglotText::ListInitializer>>;

    /**
     * @brief Construct a new Named Choice Constraint object
     * @param init the list of choices
     * @param strict should the value be constrained if not in choices
     * @param oid the oid of the constraint
     * @param shared is the constraint shared
     * @note  the first choice provided will be the default for the constraint
     */
    ChoiceConstraint(ListInitializer init, bool strict, std::string oid, bool shared)
        : choices_{init.begin(), init.end()}, strict_{strict},
          default_{init.begin()->first}, oid_{oid}, shared_{shared} {}

    /**
     * @brief Construct a new Named Choice Constraint object
     * @param init the list of choices
     * @param strict should the value be constrained if not in choices
     * @param oid the oid of the constraint
     * @param shared is the constraint shared
     * @param dm the device to add the constraint to
     * @note  the first choice provided will be the default for the constraint
     */
    ChoiceConstraint(ListInitializer init, bool strict, std::string oid, bool shared, IDevice& dm)
        : ChoiceConstraint(init, strict, oid, shared) {
        dm.addItem(oid, this);
    }

    /**
     * @brief default destructor
     */
    virtual ~ChoiceConstraint() = default;

    /**
     * @brief checks if the value satisfies the constraint
     * @param src the value to check
     * @return true if the value satisfies the constraint
     */
    bool satisfied(const st2138::Value& src) const override;

    /**
     * @brief Applies constraint to src and returns the constrained value.
     * @param src A catena::Value to apply the constraint to.
     * @return An empty catena::Value.
     *
     * If a request does not satisfy a choice constraint, then
     * the request is invalid and should be ignored.
     *
     * Calling this will always return an empty value.
     */
    st2138::Value apply(const st2138::Value& src) const override {
        return st2138::Value();
    }

    /**
     * @brief Serialize the constraint to a protobuf message.
     * @param constraint The protobuf message to populate.
     */
    void toProto(st2138::Constraint& constraint) const override;

    /**
     * @brief This constraint is not a range constraint so return false
     * @return False
     */
    bool isRange() const override { return false; }

    /**
     * @brief Checks if the constraint is shared.
     * @return True if the constraint is shared.
     */
    bool isShared() const override { return shared_; }

    /**
     * @brief Gets the constraint's oid.
     * @return The oid of the constraint.
     */
    const std::string& getOid() const override { return oid_; }

  private:
    /**
     * @brief The choices
     */
    Choices choices_; 
    /**
     * @brief Flag indicating whether the value should be constrained on apply.
     */
    bool strict_;
    /**
     * @brief The default value to constrain to.
     */
    T default_;
    /**
     * @brief Flag indicating whether the constraint is shared.
     */
    bool shared_;
    /**
     * @brief The oid of the constraint.
     */
    std::string oid_;
};

}  // namespace common
}  // namespace catena
