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
 * @file read_model_from_file.cpp
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

        // test the path class a bit
        
        catena::Path path("/one/-/two/3");
        bool going = true;
        while (going) {
            auto s1 = path.pop_front();

            if (std::holds_alternative<std::string>(s1)) {
                std::string oid(std::get<std::string>(s1));
                if (oid.compare("") == 0) {
                    going = false;
                } else {
                    std::cout << std::quoted(oid) << '\n';
                }
            }
            if (std::holds_alternative<Index>(s1)) {
                std::cout << std::get<Index>(s1) << '\n';
            }
        }


        // get some values from the device model
        float fv{};
        int iv{};
        auto fparam = dm.getValue(fv, "/hello");
        auto iparam = dm.getValue(iv, "/world");
        std::cout << "param oid: '" << dm.getOid(fparam) 
            << "' has value: " << fv
            << "\nparam oid: '" << dm.getOid(iparam)
            << "' has value: " << iv << '\n';

        // set a value in the device model
        std::cout << "setting values to something different\n";
        dm.setValue("/hello", 3.142f);
        dm.setValue(iparam, 2);

        // add a struct param the hard way
        catena::Param sparam{};
        catena::BasicParamInfo info{};
        *(info.mutable_name()->mutable_monoglot()) = "struct param";
        *(info.mutable_oid()) = "sparam";
        info.mutable_type()->set_param_type(catena::ParamType_ParamTypes::ParamType_ParamTypes_STRUCT);
        *(sparam.mutable_basic_param_info()) = info;

        catena::Value fval;
        fval.set_float32_value(1.23);
        catena::Value struct_val;
        (*(*struct_val.mutable_struct_value()->mutable_fields())["float_field"].mutable_value()) = fval;
        (*sparam.mutable_value()) = struct_val;
        dm.addParam("/sparam", std::move(sparam));

        // write out the updated device model
        std::cout << "Updated Device Model: " << dm << '\n';

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