/*
 * Copyright 2024 Ross Video Ltd
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

//common
#include <Device.h>
#include <ParamWithValue.h>
#include <PolyglotText.h>
#include <RangeConstraint.h>
#include <NamedChoiceConstraint.h>
#include <PicklistConstraint.h>

// interface
#include <interface/param.pb.h>

#include <iostream>

using namespace catena::common;
using namespace use_constraints;
using catena::common::ParamTag;
using catena::common::getParamValue;

int main() {
    // lock the model
    auto lg = dm.lock();

    catena::exception_with_status err{"", catena::StatusCode::OK};
    catena::Value value;
    std::string clientScope = "operate";


    /**
     * The button param uses an INT_CHOICE constraint to limit the valid values to 0 and 1. This
     * allows the button param to be used as a boolean. 
     * 
     * If a client tries to set the value of button to a value other than 0 or 1, then the request
     * will be ignored and the value will remain unchanged.
     */
    std::unique_ptr<IParam> ip = dm.getParam("/button", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    int32_t& button = getParamValue<int32_t>(ip.get());
    std::cout << "button initial value: " << button << "\n"; // button initial value: 0

    // This is to mimic a setValue call coming from a client
    value.set_int32_value(1); // "On"
    dm.setValue("/button", value);
    // setting button to one of the valid choices will work as usual
    std::cout << "button set to 1\n";
    std::cout << "button value: " << button << "\n"; // button value: 1

    // setting button to an invalid choice will do nothing
    value.set_int32_value(3); // "Invalid choice"
    dm.setValue("/button", value);
    std::cout << "button set to 3\n";
    std::cout << "button value: " << button << "\n" << std::endl; // button value: 1


    /**
     * The odd_numbers param uses a INT_RANGE constraint to limit the valid values to the range [0, 10], with a step size of 2.
     * This constraint will ensure that the values in the array are within the specified range.
     * 
     * If a client tries to set a value outside the range, the value will be constrained to the nearest valid value.
     * 
     * @todo enforce the step size
     */
    ip = dm.getParam("/odd_numbers", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::vector<int32_t>& odd_numbers = getParamValue<std::vector<int32_t>>(ip.get());
    std::cout << "odd_numbers initial value: ";
    for (auto& num : odd_numbers) {
        std::cout << num << " ";
    }
    std::cout << "\n"; // odd_numbers initial value: 0  2 4 6 8

    // Setting an element to a value outside the range will cause it to be constrained to the max/min values
    value.set_int32_value(-2); // below min
    err = dm.setValue("/odd_numbers/2", value);
    if (err.status != catena::StatusCode::OK) { // check for error in case the array has less than 3 elements
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    // setting odd_numbers[2] to a value below the min will cause it to be set to the min value
    std::cout << "odd_numbers[2] set to -2\n";
    std::cout << "odd_numbers[2] value: " << odd_numbers[2] << "\n"; // odd_numbers[2] value: 0

    // Constraints also apply when setting the value of the whole array
    std::vector<int32_t> newValue = {8, 12, -6, 3};
    value.mutable_int32_array_values()->mutable_ints()->Assign(newValue.begin(), newValue.end());
    dm.setValue("/odd_numbers", value);
    std::cout << "odd_numbers set to 8 12 -6 3\n";
    std::cout << "odd_numbers value: ";
    for (auto& num : odd_numbers) {
        std::cout << num << " ";
    }
    std::cout << "\n" << std::endl; // odd_numbers value: 7 9 1 3 9


    /**
     * The gain param and volume param use the same shared constraint with ref_oid: "basic_slider".
     * 
     * The basic_slider constraint is a FLOAT_RANGE constraint with a range of [0, 10]. This constraint
     * has a step size of 0.25, so the values will be constrained to multiples of 0.25.
     */
    ip = dm.getParam("/gain", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    float& gain = getParamValue<float>(ip.get());
    std::cout << "gain initial value: " << gain << std::endl; // gain initial value: 0.5
    value.set_float32_value(1.5); // within range
    dm.setValue("/gain", value);
    std::cout << "gain set to 1.5\n";
    std::cout << "gain value: " << gain << "\n"; // gain value: 1.5

    value.set_float32_value(10.5); // above max
    dm.setValue("/gain", value);
    std::cout << "gain set to 10.5\n";
    std::cout << "gain value: " << gain << "\n"; // gain value: 10

    ip = dm.getParam("/volume_array", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::vector<float>& volume = getParamValue<std::vector<float>>(ip.get());
    std::cout << "volume initial value: ";
    for (auto& vol : volume) {
        std::cout << vol << " ";
    }
    std::cout << "\n"; // volume initial value: 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0
    std::vector<float> newVolume = {0.5, 12, -4, 1, 2.1, 3.3, 4.51, 5.751};
    value.mutable_float32_array_values()->mutable_floats()->Assign(newVolume.begin(), newVolume.end());
    dm.setValue("/volume_array", value);
    std::cout << "volume set to 0.5 12 -4 1 2.1 3.3 4.51 5.751\n";
    std::cout << "volume value: ";
    for (auto& vol : volume) {
        std::cout << vol << " ";
    }
    std::cout << "\n" << std::endl; // volume value: 0.5 10.0 0.0 1.0 2.0 3.25 4.5 5.75


    /**
     * The display_size param uses a STRING_CHOICE constraint to limit the valid values to "small", "medium", and "large".
     * 
     * This constraint has the strict flag set to true, which means that the value must be one of the choices or else the 
     * setValue request will be ignored.
     */
    ip = dm.getParam("/display_size", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::string& display_size = getParamValue<std::string>(ip.get());
    std::cout << "display_size initial value: " << display_size << std::endl;
    value.set_string_value("small"); // valid choice
    dm.setValue("/display_size", value);
    std::cout << "display_size set to small\n";
    std::cout << "display_size value: " << display_size << "\n"; // display_size value: small

    value.set_string_value("tiny"); // invalid choice
    dm.setValue("/display_size", value);
    std::cout << "display_size set to tiny\n";
    std::cout << "display_size value: " << display_size << "\n" << std::endl; // display_size value: small


    /**
     * The image param uses a STRING_STRING_CHOICE constraint to suggest valid choices for the image param 
     * to the client. The display strings of a STRING_STRING_CHOICE constraint are only used for display 
     * purposes and do not affect the value of the param. 
     * 
     * The strict flag is set to false, which means that the client can set the value to any string, even 
     * if it is not in the list of choices.
     */
    ip = dm.getParam("/image", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::string& image = getParamValue<std::string>(ip.get());
    std::cout << "image initial value: " << image << std::endl;
    value.set_string_value("eo://dog.png"); // valid choice
    dm.setValue("/image", value);
    std::cout << "image set to dog\n";
    std::cout << "image value: " << image << "\n"; // image value: eo://dog.png

    value.set_string_value("eo://bird.png"); // invalid choice
    dm.setValue("/image", value);
    std::cout << "image set to bird\n";
    std::cout << "image value: " << image << "\n" << std::endl; // image value: eo://bird.png


    /**
     * The button_arrray param has the template_oid: "/button" which means that the button_array 
     * param inherits the constraint from the button param. This means that each element in the
     * button_array must be either 0 or 1.
     * 
     * setting any element in the button_array to a value other than 0 or 1 will cause the value to be
     * ignored for that element.
     */
    ip = dm.getParam("/button_array", err);
    if (ip == nullptr){
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::vector<int32_t>& button_array = getParamValue<std::vector<int32_t>>(ip.get());
    std::cout << "button_array initial value: ";
    for (auto& num : button_array) {
        std::cout << num << " ";
    }
    std::cout << "\n"; // button_array initial value: 0 0 0 0

    /**
     * If the incomming value has more elemnts than the button_array, the extra elements will be appended
     * to the end of the array only if they are valid values.
     */
    std::vector<int32_t> arrayVal = {0, 1, -1, 2, 2, 1};
    value.mutable_int32_array_values()->mutable_ints()->Assign(arrayVal.begin(), arrayVal.end());
    dm.setValue("/button_array", value);
    std::cout << "button_array set to 0 1 -1 2 2 1\n";
    std::cout << "button_array value: ";
    for (auto& num : button_array) {
        std::cout << num << " ";
    }
    std::cout << "\n" << std::endl; // button_array value: 0 1 0 0 1

    return EXIT_SUCCESS;
}

/* possible output
button initial value: 0
button set to 1
button value: 1
button set to 3
button value: 1

odd_numbers initial value: 1 3 5 7 9 
odd_numbers[2] set to -2
odd_numbers[2] value: 1
odd_numbers set to 8 12 -6 3
odd_numbers value: 7 9 1 3 9 

gain initial value: 0.5
gain set to 1.5
gain value: 1.5
gain set to 10.5
gain value: 10
volume initial value: 0 0 0 0 0 0 0 0 
volume set to 0.5 12 -4 1 2.1 3.3 4.51 5.751
volume value: 0.5 10 0 1 2 3.25 4.5 5.75 

display_size initial value: medium
display_size set to small
display_size value: small
display_size set to tiny
display_size value: small

image initial value: eo://cat.png
image set to dog
image value: eo://dog.png
image set to bird
image value: eo://bird.png

button_array initial value: 0 0 0 0 
button_array set to 0 1 -1 2 2 1
button_array value: 0 1 0 0 1 
*/