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
 * @file use_constraints.cpp
 * @author isaac.robert@rossvideo.com
 * @brief showcases the use of constraints in the model
 *
 * It does not support any connections so is not a complete example
 * of a working device.
 *
 * It presumes the reader has understood the start_here example and
 * builds on that. Less chatty comments.
 * 
 * @copyright Copyright (c) 2024 Ross Video
 */

// device model
#include "device.use_constraints.json.h"

//lite
#include <Device.h>
#include <ParamWithValue.h>
#include <PolyglotText.h>
#include <RangeConstraint.h>
#include <NamedChoiceConstraint.h>
#include <PicklistConstraint.h>

// interface
#include <interface/param.pb.h>

#include <iostream>

using namespace catena::lite;
using namespace catena::common;
using namespace use_constraints;
using catena::common::ParamTag;

int main() {
    // lock the model
    Device::LockGuard lg(dm);

    IParam* ip = dm.getItem<ParamTag>("display");
    assert(ip != nullptr);
    auto& displayParam = *dynamic_cast<ParamWithValue<std::string>*>(ip);
    // @todo example needs proto serialization to use apply

    return EXIT_SUCCESS;
}