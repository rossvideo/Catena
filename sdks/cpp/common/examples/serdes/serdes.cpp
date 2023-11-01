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
#include <Path.h>

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <utility>

using Index = catena::Path::Index;

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
        catena::DeviceModel<catena::Threading::kSingleThreaded> dm(argv[1]);

        

        // write the device model to stdout
        std::cout << "Read Device Model: " << dm << '\n';

        // report the wire-size of the device model
        std::string serialized;
        dm.device().SerializeToString(&serialized);
        std::cout << "Device model serializes to " << serialized.size() << " bytes\n";

    } catch (std::exception& why) {
        std::cerr << "Problem: " << why.what() << '\n';
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}