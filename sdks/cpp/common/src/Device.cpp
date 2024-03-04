/**
 * @brief Analog to catena::Device
 * @file Device.cpp
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

#include <Device.h>
#include <DeviceModel.h>
#include <utils.h>

using catena::sdk::Device;
using ParamsMap = catena::DeviceModel::ParamsMap;

static auto jpopts = google::protobuf::util::JsonParseOptions{};

Device::Device(const std::string& filename) {
    // read in the top level file, code block used to minimize
    // lifespan of the imported file
    {
        catena::Device pbufDevice{};
        std::string file = catena::readFile(filename);
        auto status = google::protobuf::util::JsonStringToMessage(file, &pbufDevice, jpopts);
        if (!status.ok()) {
            std::stringstream err;
            err << "error parsing " << filename << ": " << status.message();
            BAD_STATUS(err.str(), catena::StatusCode::INVALID_ARGUMENT);
        }
        *this = pbufDevice;
    }
    // read in imported params
    // the top-level ones will come from path/to/device/params
    //
    std::filesystem::path current_folder(filename);
    current_folder = current_folder.remove_filename() /= std::string("params");
    importSubParams_(current_folder, params_);
}

void Device::importSubParams_(std::filesystem::path &current_folder, ParamsMap &params) {
    for (auto it = params.begin(); it != params.end(); ++it) {
        catena::sdk::IParam& child = *it->second;
        if (child.include()) {
            // this is a local import, use the oid to create the filename
            std::filesystem::path to_import(current_folder);
            std::stringstream fn;
            std::string imported;
            fn << "param." << it->first << ".json";
            to_import /= fn.str();
            std::cout << "importing: " << to_import << '\n';

            // import the file
            imported = readFile(to_import);
            // clear the "import" typing of the current param
            // and overwrite with what we just imported
            catena::Param pbufParam{};
            auto status = google::protobuf::util::JsonStringToMessage(imported, &pbufParam, jpopts);
            if (!status.ok()) {
                std::stringstream err;
                err << "error importing " << to_import << ": " << status.message();
                BAD_STATUS(err.str(), catena::StatusCode::INVALID_ARGUMENT);
            }
            child = pbufParam;

            // recurse to import any sub-params
            if (child.hasSubparams()) {
                std::filesystem::path next_folder(current_folder);
                next_folder = next_folder /= it->first;
                importSubParams_(next_folder, child.subparams());
            }
        }
    }
}

Device& Device::operator=(const catena::Device& pbufDevice) {
    for (const auto& [oid, param] : pbufDevice.params()) {
        addParam(oid, param);
    }
    return *this;

}

void Device::addParam(const std::string& oid, const catena::Param& param) {
    auto& fac = catena::sdk::IParam::Factory::getInstance();
    auto p = fac.makeProduct(param.value().kind_case(), param);
    params_[oid] = p;
}

