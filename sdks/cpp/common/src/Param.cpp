/**
 * @brief Analog to catena::Param
 * @file Param.cpp
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

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

#include <Param.h>
using catena::sdk::IParam;
using catena::sdk::ParamCommon;
using catena::sdk::Param;


/* Param Common Definitions */
ParamCommon::ParamCommon(const catena::Param& src) : type_{src.type()}, import_{src.import()} {}

ParamCommon& ParamCommon::operator=(const catena::Param& src) {
    type_ = src.type();
    import_ = src.import();
    return *this;
}

bool ParamCommon::include() const {
    // only test for include directive if not already included
    return included_ || import_.isIncludeDirective();
}

void ParamCommon::included() {
    import_ = Import{}; // remove include directive
    included_ = true;   // mark as included
}


/* Param Definitions */

using Int32Param = Param<int32_t>;

template <>
Int32Param::Initializer Int32Param::initializer_;
template <>
bool Int32Param::registered_ = false;