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
 * @file use_structs.cpp
 * @author john.naylor@rossvideo.com
 * @brief Steps up the complexity a notch by dealing with structured data
 *
 * It does not support any connections so is not a complete example
 * of a working device.
 *
 * It presumes the reader has understood the start_here example and
 * builds on that. Less chatty comments.
 * 
 * @copyright Copyright © 2024 Ross Video Ltd
 */

// device model
#include "device.AudioDeck.json.h" 

// common
#include <Device.h>
#include <ParamWithValue.h>
#include <PolyglotText.h>

// protobuf interface
#include <interface/param.pb.h>

using namespace catena::common;
using namespace AudioDeck;
using catena::common::ParamTag;
#include <iostream>
int main() {
    // lock the model
    Device::LockGuard lg(dm);
    catena::exception_with_status err{"", catena::StatusCode::OK};

    std::unique_ptr<IParam> ip = dm.getParam("/audio_deck", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    catena::Value value;
    std::string clientScope = "operate";
    ip->toProto(value, clientScope);
    std::cout << "audio_deck: " << value.DebugString() << std::endl;

    // this line is for demonstrating the fromProto method
    // this should never be done in a real device
    value.mutable_struct_array_values()->mutable_struct_values()->at(2).mutable_fields()->at("eq_list").mutable_value()->mutable_struct_array_values()->mutable_struct_values()->at(1).mutable_fields()->at("q_factor").mutable_value()->set_float32_value(2.5);
    ip->fromProto(value, clientScope);

    ip = dm.getParam("/audio_deck/2", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    value.Clear();
    clientScope = "operate";
    ip->toProto(value, clientScope);
    std::cout << "audio_deck[2]: " << value.DebugString() << std::endl;


    // add a new audio channel to audio_deck
    ip = dm.getParam("/audio_deck/-", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    value.Clear();
    clientScope = "operate";
    ip->toProto(value, clientScope);
    std::cout << "new audio channel: " << value.DebugString() << std::endl;

    ip = dm.getParam("/audio_deck/3/eq_list/0/response", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    value.Clear();
    clientScope = "operate";
    ip->toProto(value, clientScope);
    std::cout << "/audio_deck/1/eq_list/1/response: " << value.DebugString() << std::endl;

    ip = dm.getParam("/audio_deck/2/eq_list/1/q_factor", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    value.Clear();
    clientScope = "operate";
    ip->toProto(value, clientScope);
    std::cout << "/audio_deck/2/eq_list/1/q_factor: " << value.DebugString() << std::endl;

    return EXIT_SUCCESS;
}