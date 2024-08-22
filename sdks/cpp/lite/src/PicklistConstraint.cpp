#include <PicklistConstraint.h>

#include <lite/param.pb.h>

namespace catena {
namespace lite {

PicklistConstraint::PicklistConstraint(ListInitializer init, bool strict, std::string oid, 
    bool shared, Device& dm)
    : IConstraint{oid, shared}, choices_{init.begin(), init.end()}, 
    strict_{strict}, default_{*init.begin()} {
        dm.addItem<common::ConstraintTag>(oid, this);
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