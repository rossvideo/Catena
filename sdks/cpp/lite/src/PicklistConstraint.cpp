// Licensed under the Creative Commons Attribution NoDerivatives 4.0
// International Licensing (CC-BY-ND-4.0);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
// https://creativecommons.org/licenses/by-nd/4.0/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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