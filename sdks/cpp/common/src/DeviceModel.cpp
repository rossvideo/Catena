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

#include <DeviceModel.h>
#include <ParamAccessor.h>
#include <Path.h>
#include <utils.h>

#include <google/protobuf/map.h>
#include <google/protobuf/util/json_util.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <tuple>


using catena::DeviceModel;
using catena::ParamAccessor;
using catena::Threading;
using google::protobuf::Map;

using grpc::ServerWriter;
using grpc::Status;

 DeviceModel::DeviceModel(const std::string &filename) : device_{} {
    /** @todo recurse into parameters, implementation currently only works 1-layer
   * deep*/

    // initialize static attributes
    noValue_.set_undefined_value(catena::UndefinedValue());
    auto jpopts = google::protobuf::util::JsonParseOptions{};
    // read in the top level file
    {
        std::string file = catena::readFile(filename);
        auto status = google::protobuf::util::JsonStringToMessage(file, &device_, jpopts);
        if (!status.ok()) {
            std::stringstream err;
            err << "error parsing " << filename << ": " << status.message();
            BAD_STATUS(err.str(), catena::StatusCode::INVALID_ARGUMENT);
        }
    }

    // read in imported params
    // the top-level ones will come from path/to/device/params
    //
    std::filesystem::path current_folder(filename);
    current_folder = current_folder.remove_filename() /= "params";

    for (auto it = device_.mutable_params()->begin(); it != device_.mutable_params()->end(); ++it) {
        std::cout << it->first << '\n';
        catena::Param &pdesc = device_.mutable_params()->at(it->first);
        if (pdesc.has_import()) {
            if (pdesc.import().url().compare("include") == 0) {
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
                pdesc.clear_import();
                auto status = google::protobuf::util::JsonStringToMessage(imported, &pdesc, jpopts);
                if (!status.ok()) {
                    std::stringstream err;
                    err << "error importing " << to_import << ": " << status.message();
                    BAD_STATUS(err.str(), catena::StatusCode::INVALID_ARGUMENT);
                }
            }
            else if (pdesc.import().url().compare("") != 0) {
                /** @todo implement url imports*/
                BAD_STATUS("Cannot (yet) import from urls, sorry.", catena::StatusCode::UNIMPLEMENTED);
            }
        }
    }
}

const catena::Device &catena::DeviceModel::device() const {
    LockGuard lock(mutex_);
    return device_;
}

//send device info to client via writer
void catena::DeviceModel::streamDevice(ServerWriter< ::catena::DeviceComponent> *writer){
    LockGuard lock(mutex_);
    catena::DeviceComponent dc;
    dc.set_allocated_device(&device_);
    writer->Write(dc);
    auto x = dc.release_device();
}

// for parameters that do not have values
catena::Value catena::DeviceModel::noValue_;

catena::ParamAccessor catena::DeviceModel::param(const std::string &jptr) {
    LockGuard lock(mutex_);
    catena::Path path_(jptr);

    // get our oid and look for it in the params map
    catena::Path::Segment segment = path_.pop_front();

    if (!std::holds_alternative<std::string>(segment)) {
        BAD_STATUS("expected oid, got an index", catena::StatusCode::INVALID_ARGUMENT);
    }
    std::string oid(std::get<std::string>(segment));

    if (!device_.mutable_params()->contains(oid)) {
        std::stringstream msg;
        msg << "param " << std::quoted(oid) << " not found";
        BAD_STATUS(msg.str(), catena::StatusCode::NOT_FOUND);
    }

    ParamAccessorData ans;
    catena::Param &p = device_.mutable_params()->at(oid);
    std::get<0>(ans) = &p;
    std::get<1>(ans) = (p.has_value() ? p.mutable_value() : &noValue_);

    while (path_.size()) {
        if (std::holds_alternative<std::string>(path_.front()) ) {
            ans = getSubparam_(path_, ans);
        } else if (std::holds_alternative<std::deque<std::string>::size_type>(path_.front())) {
            ans = indexIntoParam_(path_, ans);
        } else {
            BAD_STATUS("expected oid or index", catena::StatusCode::INVALID_ARGUMENT);
        }
    }

    return ParamAccessor(*this, ans);
}


typename catena::DeviceModel::ParamAccessorData
catena::DeviceModel::indexIntoParam_(catena::Path &path, catena::DeviceModel::ParamAccessorData &pad) {

    // destructure the param-value pair
    auto [parent, value] = pad;

    auto idx = std::get<0>(path.pop_front());
    catena::Value *v = &noValue_;
    
    if (!parent->has_value()) {
        BAD_STATUS("param does not have a value", catena::StatusCode::FAILED_PRECONDITION);
    }

    // validate the param type
    catena::ParamType_Type type = parent->type().type();
    switch (type) {
        case catena::ParamType_Type::ParamType_Type_FLOAT32_ARRAY:
            if (!value->has_float32_array_values()) {
                BAD_STATUS("value must be of type float32_array_value",
                        catena::StatusCode::FAILED_PRECONDITION);
            }

            if (idx >= value->float32_array_values().floats_size()) {
                // range error
                std::stringstream err;
                err << "Index is out of range: " << idx << " >= " << value->float32_array_values().floats_size();
                BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
            }
            v->set_float32_value(value->mutable_float32_array_values()->floats(idx));
            break;
        case catena::ParamType_Type::ParamType_Type_STRUCT_ARRAY:
            if (!value->has_struct_array_values()) {
                BAD_STATUS("value must be of type struct_array_value",
                        catena::StatusCode::FAILED_PRECONDITION);
            }

            if (idx >= value->struct_array_values().struct_values_size()) {
                // range error
                std::stringstream err;
                err << "Index is out of range: " << idx << " >= " << value->struct_array_values().struct_values_size();
                BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
            }
            v->mutable_struct_value()->CopyFrom(value->mutable_struct_array_values()->struct_values(idx));
            break;
        case catena::ParamType_Type::ParamType_Type_STRING_ARRAY:
            if (!value->has_string_array_values()) {
                BAD_STATUS("value must be of type string_array_value",
                        catena::StatusCode::FAILED_PRECONDITION);
            }

            if (idx >= value->string_array_values().strings_size()) {
                // range error
                std::stringstream err;
                err << "Index is out of range: " << idx << " >= " << value->string_array_values().strings_size();
                BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
            }
            v->set_string_value(value->mutable_string_array_values()->strings(idx));
            break;
        case catena::ParamType_Type::ParamType_Type_INT32_ARRAY:
            if (!value->has_int32_array_values()) {
                BAD_STATUS("value must be of type int32_array_value",
                        catena::StatusCode::FAILED_PRECONDITION);
            }

            if (idx >= value->int32_array_values().ints_size()) {
                // range error
                std::stringstream err;
                err << "Index is out of range: " << idx << " >= " << value->int32_array_values().ints_size();
                BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
            }
            v->set_int32_value(value->mutable_int32_array_values()->ints(idx));
            break;
        default:
            std::stringstream err;
            err << "cannot index into param of type: " << type;
            BAD_STATUS((err.str()), catena::StatusCode::INVALID_ARGUMENT);
    }    

    return ParamAccessorData(parent, v);
}

typename catena::DeviceModel::ParamAccessorData
catena::DeviceModel::getSubparam_(catena::Path &path, catena::DeviceModel::ParamAccessorData &pad) {

    // destructure the param-value pair
    auto [parent, value] = pad;

    // validate the param type
    catena::ParamType_Type type = parent->type().type();
    switch (type) {
        case catena::ParamType_Type::ParamType_Type_STRUCT:
            // this is ok
            break;
        case catena::ParamType_Type::ParamType_Type_STRUCT_ARRAY:
            // this is ok
            break;
        default:
            std::stringstream err;
            err << "cannot sub-param param of type: " << type;
            BAD_STATUS((err.str()), catena::StatusCode::INVALID_ARGUMENT);
    }

    // validate path argument and the parameter

    // is there a params field to define the sub-params?
    if (parent->params_size() == 0) {
        BAD_STATUS("params field is missing", catena::StatusCode::FAILED_PRECONDITION);
    }

    // is our oid a string?
    auto seg = path.pop_front();
    if (!std::holds_alternative<std::string>(seg)) {
        BAD_STATUS("expected oid, got index", catena::StatusCode::INVALID_ARGUMENT);
    }

    // is the oid defined?
    std::string oid(std::get<std::string>(seg));
    if (!parent->params().contains(oid)) {
        std::stringstream err;
        err << oid << " not found";
        BAD_STATUS((err.str()), catena::StatusCode::FAILED_PRECONDITION);
    }

    // descend the value tree to find the value matching the oid
    catena::Value *v;
    if (value != &noValue_) {
        if (!(value->has_struct_value() || value->has_struct_array_values())) {
            BAD_STATUS("value must be of type struct_value or struct_array_value",
                       catena::StatusCode::FAILED_PRECONDITION);
        }

        if (value->has_struct_value()) {
            if (value->struct_value().fields_size() == 0) {
                // no fields are defined, so terminate further descent.
                v = &noValue_;
            } else if (!value->struct_value().fields().contains(oid)) {
                // the value for this field isn't present, which is ok
                // but terminate further descent
                v = &noValue_;
            } else {
                v = value->mutable_struct_value()->mutable_fields()->at(oid).mutable_value();
            }
        }

        if (value->has_struct_array_values()) {
            /** @todo implement value navigation for STRUCT_ARRAY */
            BAD_STATUS("value navigation for struct_array_value not implemented, sorry",
                       catena::StatusCode::UNIMPLEMENTED);
        }
    }

    catena::Param &p = parent->mutable_params()->at(oid);
    return ParamAccessorData(&p, v);
}

std::ostream &operator<<(std::ostream &os, const catena::DeviceModel &dm) {
    os << printJSON(dm.device());
    return os;
}