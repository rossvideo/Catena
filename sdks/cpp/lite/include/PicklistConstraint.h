#pragma once

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

/**
 * @file PicklistConstraint.h
 * @brief A constraint that checks if a value is in a list of strings
 * @author isaac.robert@rossvideo.com
 * @date 2024-08-09
 * @copyright Copyright (c) 2024 Ross Video
 */

// common
#include <IConstraint.h>
#include <IParam.h>
#include <Tags.h>

// lite
#include <Device.h>

#include <google/protobuf/message_lite.h>

#include <string>
#include <unordered_set>
#include <initializer_list>

namespace catena {
namespace lite {

/**
 * @brief Picklist constraint, ensures a value is in a list of strings
 */
class PicklistConstraint : public catena::common::IConstraint {
public:
    /**
     * @brief map of choices with their display names
     */
    using Choices = std::unordered_set<std::string>;
    /**
     * @brief initializer list for choices
     */
    using ListInitializer = std::initializer_list<std::string>;

public:
    /**
     * @brief Construct a new Picklist Constraint object
     * @param init the list of choices
     * @param strict should the value be constrained if not in choices
     * @param oid the oid of the constraint
     * @param shared is the constraint shared
     * @param dm the device to add the constraint to
     * @note  the first choice provided will be the default for the constraint
     */
    PicklistConstraint(ListInitializer init, bool strict, std::string oid, bool shared, Device& dm);
    
    /**
     * @brief Construct a new Picklist Constraint object
     * @param init the list of choices
     * @param strict should the value be constrained if not in choices
     * @param oid the oid of the constraint
     * @param shared is the constraint shared
     * @param parent the param to add the constraint to
     * @note  the first choice provided will be the default for the constraint
     */
    PicklistConstraint(ListInitializer init, bool strict, std::string oid, bool shared, catena::common::IParam* parent);

    /**
     * @brief default destructor
     */
    virtual ~PicklistConstraint();

    /**
     * @brief applies choice constraint to a catena::Value if strict
     * @param src a catena::Value to apply the constraint to
     */
    void apply(void* src) const override;

    /**
     * @brief serialize the constraint to a protobuf message
     * @param msg the protobuf message to populate
     */
    void toProto(google::protobuf::MessageLite& msg) const override;

private:
    Choices choices_;     ///< the choices
    bool strict_;         ///< should the value be constrained on apply
    std::string default_; ///< the default value to constrain to
};

} // namespace lite
} // namespace catena