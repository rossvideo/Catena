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
 * @file use_templates.cpp
 * @author john.danen@rossvideo.com
 * @brief Shows how to use templates to create multiple struct params of the same type.
 *
 * It does not support any connections so is not a complete example
 * of a working device.
 *
 * It presumes the reader has understood the start_here and use_structs examples and
 * builds on that. Less chatty comments.
 * 
 * @copyright Copyright © 2024 Ross Video Ltd
 */

// device model
#include "device.use_templates.json.h"  

// lite
#include <Device.h>
#include <ParamWithValue.h>
#include <ParamDescriptor.h>
#include <PolyglotText.h>

// protobuf interface
#include <interface/param.pb.h>

using namespace catena::lite;
using namespace catena::common;
using namespace use_templates;
#include <iostream>
int main() {
    // lock the model
    Device::LockGuard lg(dm);

    IParam* ip = dm.getItem<ParamTag>("ottawa");
    assert(ip != nullptr);
    auto& canadasCapital = *dynamic_cast<ParamWithValue<City>*>(ip);
    City& city = canadasCapital.get();
    std::cout << "Canada's capital city is " << city.city_name
              << " at latitude " << city.latitude
              << " and longitude " << city.longitude
              << " with a population of " << city.population
              << std::endl;

    ip = dm.getItem<ParamTag>("toronto");
    auto& ontariosCapital = *dynamic_cast<ParamWithValue<City>*>(ip);
    City& city2 = ontariosCapital.get();
    std::cout << "Ontario's capital city is " << city2.city_name
              << " at latitude " << city2.latitude
              << " and longitude " << city2.longitude
              << " with a population of " << city2.population
              << std::endl;
   
    return EXIT_SUCCESS;
}