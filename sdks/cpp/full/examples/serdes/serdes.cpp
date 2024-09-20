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
 * @brief Reads a catena device model from a JSON file and writes it to stdout.
 *
 * Design intent: provide a handy way to validate (potentially) human-
 * authored device models. If the model is empty, the input is faulty.
 *
 * Note that items in the input model that have default values (0 for ints,
 * false for booleans, ...) will be stripped from the model that's output.
 *
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * @file serdes.cpp
 * @copyright Copyright © 2023, Ross Video Ltd
 */

#include <DeviceModel.h>
#include <ParamAccessor.h>
#include <Path.h>

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <utility>

#include <fcntl.h>
#include <unistd.h>

using Index = catena::common::Path::Index;

int main(int argc, char** argv) {
    // process command line
    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " path/to/input-file.json\n";
        exit(EXIT_SUCCESS);
    }

    try {

        // read a json file into a DeviceModel object
        // we don't need this one to be threadsafe, so use false
        // as the template parameter
        catena::full::DeviceModel dm(argv[1]);

        // write the device model to stdout
        std::cout << "Read Device Model: " << dm << '\n';

        // report the wire-size of the device model
        std::string serialized;
        dm.device().SerializeToString(&serialized);
        std::cout << "Device model serializes to " << serialized.size() << " bytes\n";

        // write the device model to a file
        // Open the file for writing and get a file descriptor
        int fd = open("model.bin", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            std::cerr << "Failed to open file\n";
            exit(EXIT_FAILURE);
        }

        // write the device model to the file
        if(!dm.device().SerializeToFileDescriptor(fd)) {
            std::cerr << "Failed to write to file\n";
            exit(EXIT_FAILURE);
        }

        // Don't forget to close the file descriptor when you're done
        close(fd);

    } catch (std::exception& why) {
        std::cerr << "Problem: " << why.what() << '\n';
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}