#pragma once

/**
 * @file NamedChoiceConstraint.h
 * @brief A constraint that checks if a value is within a named choice
 * @author isaac.robert@rossvideo.com
 * @date 2024-07-25
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <common/include/IConstraint.h>
#include <google/protobuf/message_lite.h>
#include <lite/include/PolyglotText.h>

using catena::common::IConstraint;
using catena::lite::PolyglotText;

/**
 * @brief Named choice constraint, ensures a value is within a named choice.
 */
template <typename T> 
class NamedChoiceConstraint : public IConstraint {
public:
    /**
     * @brief Construct a new Named Choice Constraint object
     * @param init the list of choices
     */
    NamedChoiceConstraint(std::vector<std::pair<T, PolyglotText::ListInitializer>> init,
        bool strict, std::string oid, bool shared)
        : strict_{strict}, default_{init[0].first} {
        for (size_t i = 0; i < init.size(); ++i) {
            choices_[init[i].first] = init[i].second;
        }
        setOid(oid);
        setShared(shared);
    }

    /**
     * @brief set the value in dst to the value in src if the value in src is valid 
     * @param dst the value to write the constrained value to
     * @param src the value to read and apply the constraint on
     */
    void apply(void* dst, void* src) const override {
        auto& update = *reinterpret_cast<catena::Value*>(src);
        
        if constexpr(std::is_same<T, int32_t>::value) {
            // ignore the request if src is not valid
            if (!update.has_int32_value()) { return; }

            // constrain the choice if strict 
            if (strict_ && !choices_.contains(update.int32_value())) {
                update.set_int32_value(default_);
            }
            reinterpret_cast<catena::Value*>(dst)->set_int32_value(update.int32_value());
        }

        if constexpr(std::is_same<T, std::string>::value) {
            // ignore the request if src is not valid
            if (!update.has_string_value()) { return; }

            // constrain the choice if strict 
            if (strict_ && !choices_.contains(update.float32_value())) {
                update.set_float32_value(default_);
            }
            reinterpret_cast<catena::Value*>(dst)->set_string_value(update.string_value());
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
    std::unordered_map<T, PolyglotText> choices_; ///< the choices
    bool strict_;                                 ///< should the value be constrained on apply
    const T default_;                             ///< the default value to constrain to
};
