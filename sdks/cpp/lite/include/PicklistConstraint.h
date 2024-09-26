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