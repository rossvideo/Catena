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