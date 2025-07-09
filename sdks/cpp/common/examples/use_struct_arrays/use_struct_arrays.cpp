/*
 * Copyright 2025 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
 * @copyright Copyright © 2025 Ross Video Ltd
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
    std::lock_guard lg(dm.mutex());
    catena::exception_with_status err{"", catena::StatusCode::OK};

    std::unique_ptr<IParam> ip = dm.getParam("/audio_deck", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    catena::Value value;
    ip->toProto(value, Authorizer::kAuthzDisabled);
    std::cout << "audio_deck: " << value.DebugString() << std::endl;

    // this line is for demonstrating the fromProto method
    // this should never be done in a real device
    value.mutable_struct_array_values()->mutable_struct_values()->at(2).mutable_fields()->at("eq_list").mutable_struct_array_values()->mutable_struct_values()->at(1).mutable_fields()->at("q_factor").set_float32_value(2.5);
    ip->fromProto(value, Authorizer::kAuthzDisabled);

    ip = dm.getParam("/audio_deck/2", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    value.Clear();
    ip->toProto(value, Authorizer::kAuthzDisabled);
    std::cout << "audio_deck[2]: " << value.DebugString() << std::endl;


    // add a new audio channel to audio_deck
    // getParam /- is invalid.
    value.Clear();
    err = dm.setValue("/audio_deck/-", value);
    if (err.status != catena::StatusCode::OK){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "new audio channel: " << value.DebugString() << std::endl;

    ip = dm.getParam("/audio_deck/3/eq_list/0/response", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    value.Clear();
    ip->toProto(value, Authorizer::kAuthzDisabled);
    std::cout << "/audio_deck/1/eq_list/1/response: " << value.DebugString() << std::endl;

    ip = dm.getParam("/audio_deck/2/eq_list/1/q_factor", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    value.Clear();
    ip->toProto(value, Authorizer::kAuthzDisabled);
    std::cout << "/audio_deck/2/eq_list/1/q_factor: " << value.DebugString() << std::endl;

    return EXIT_SUCCESS;
}

/* Possible Output
audio_deck: MessageLite at 0x7ffe14a5ac70
audio_deck[2]: MessageLite at 0x7ffe14a5ac70
new audio channel: MessageLite at 0x7ffe14a5ac70
/audio_deck/1/eq_list/1/response: MessageLite at 0x7ffe14a5ac70
/audio_deck/2/eq_list/1/q_factor: MessageLite at 0x7ffe14a5ac70
*/