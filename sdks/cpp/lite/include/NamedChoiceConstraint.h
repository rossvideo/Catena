#pragma once

/**
 * @file NamedChoiceConstraint.h
 * @brief A constraint that checks if a value is within some named choices
 * @author isaac.robert@rossvideo.com
 * @date 2024-07-25
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <common/include/IConstraint.h>
#include <google/protobuf/message_lite.h>
#include <lite/include/PolyglotText.h>
#include <lite/include/Collection.h>

#include <string>
#include <unordered_map>

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
     * @brief local alias for IConstraint
     */
    using IConstraint = catena::common::IConstraint;
    /**
     * @brief local alias for PolyglotText
     */
    using PolyglotText = catena::lite::PolyglotText;
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
     * @note  the first choice provided will be the default for the constraint
     */
    NamedChoiceConstraint(ListInitializer init, bool strict, std::string oid, bool shared, Collection<IConstraint>& parent)
        : IConstraint{oid, shared}, choices_{init.begin(), init.end()}, 
        strict_{strict}, default_{init.begin()->first} {
            parent.addItem(oid, this);
        }

    /**
     * @brief default destructor
     */
    virtual ~NamedChoiceConstraint() = default;

    /**
     * @brief applies choice constraint to a catena::Value if strict
     * @param src a catena::Value to apply the constraint to
     */
    void apply(void* src) const override {
        auto& src_val = *reinterpret_cast<catena::Value*>(src);

        if constexpr(std::is_same<T, int32_t>::value) {
            // ignore the request if src is not valid
            if (!src_val.has_int32_value()) { return; }

            // constrain if strict and src is not in choices
            if (strict_ && !choices_.contains(src_val.int32_value())) {
                src_val.set_int32_value(default_);
            }
        }

        if constexpr(std::is_same<T, std::string>::value) {
            // ignore the request if src is not valid
            if (!src_val.has_string_value()) { return; }

            // constrain if strict and src is not in choices
            if (strict_ && !choices_.contains(src_val.string_value())) {
                src_val.set_string_value(default_);
            }
        } 
    }

    /**
     * @brief serialize the constraint to a protobuf message
     * @param msg the protobuf message to populate
     */
    void toProto(google::protobuf::MessageLite& msg) const override {
        auto& constraint = dynamic_cast<catena::Constraint&>(msg);

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

private:
    Choices choices_; ///< the choices
    bool strict_;     ///< should the value be constrained on apply
    T default_;       ///< the default value to constrain to
};

} // namespace lite
} // namespace catena