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
 */


#include "lite/examples/use_constraints/device.use_constraints.json.h"  // dm
#include <lite/include/Device.h>
#include <lite/include/ParamWithValue.h>
#include <lite/include/PolyglotText.h>
#include <lite/include/RangeConstraint.h>
#include <lite/include/NamedChoiceConstraint.h>
#include <lite/include/PicklistConstraint.h>
#include <lite/param.pb.h>

using namespace catena::lite;
using namespace catena::common;
using namespace use_constraints;
using catena::common::ParamTag;
#include <iostream>
int main() {
    // lock the model
    Device::LockGuard lg(dm);

    IParam* ip = dm.getItem<ParamTag>("display");
    assert(ip != nullptr);
    auto& displayParam = *dynamic_cast<ParamWithValue<std::string>*>(ip);
    // @todo example needs proto serialization to use apply

    return EXIT_SUCCESS;
}