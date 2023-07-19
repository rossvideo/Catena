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

template <enum Threading T> DeviceModel<T>::DeviceModel(const std::string &filename) : device_{} {
    /** @todo recurse into parameters, implementation currently only works 1-layer
   * deep*/

    // initialize static attributes
    noValue_.set_undefined_value(catena::UndefinedValue());
    auto jpopts = google::protobuf::util::JsonParseOptions{};
    // read in the top level file
    {
        std::string file = catena::readFile(filename);
        google::protobuf::util::JsonStringToMessage(file, &device_, jpopts);
    }

    // read in imported params
    // the top-level ones will come from path/to/device/params
    //
    std::filesystem::path current_folder(filename);
    current_folder = current_folder.remove_filename() /= "params";

    for (auto it = device_.mutable_params()->begin(); it != device_.mutable_params()->end(); ++it) {
        std::cout << it->first << '\n';
        catena::ParamDescriptor &pdesc = device_.mutable_params()->at(it->first);
        if (pdesc.has_import()) {
            std::string imported;
            if (pdesc.import().url().compare("") != 0) {
                /** @todo implement url imports*/
                BAD_STATUS("Cannot (yet) import from urls, sorry.", grpc::StatusCode::UNIMPLEMENTED);
            } else {
                // there's no url specified, so do a local import using the oid
                // to create the filename
                std::filesystem::path to_import(current_folder);
                std::stringstream fn;
                /** @todo escape the filename, it->first in case it includes a solidus
         */
                fn << "param." << it->first << ".json";
                to_import /= fn.str();
                std::cout << "importing: " << to_import << '\n';

                // import the file
                imported = readFile(to_import);
            }

            // clear the "import" typing of the current param
            // and overwrite with what we just imported
            pdesc.clear_import();
            google::protobuf::util::JsonStringToMessage(imported, pdesc.mutable_param(), jpopts);
        }
    }
}

template <enum Threading T> const catena::Device &catena::DeviceModel<T>::device() const {
    LockGuard lock(mutex_);
    return device_;
}

// for parameters that do not have values
template <enum Threading T> catena::Value catena::DeviceModel<T>::noValue_;

template <enum Threading T>
catena::ParamAccessor<catena::DeviceModel<T>> catena::DeviceModel<T>::param(const std::string &jptr) {
    LockGuard lock(mutex_);
    catena::Path path_(jptr);

    // get our oid and look for it in the params map
    catena::Path::Segment segment = path_.pop_front();

    if (!std::holds_alternative<std::string>(segment)) {
        BAD_STATUS("expected oid, got an index", grpc::StatusCode::INVALID_ARGUMENT);
    }
    std::string oid(std::get<std::string>(segment));

    if (!device_.mutable_params()->contains(oid)) {
        std::stringstream msg;
        msg << "param " << std::quoted(oid) << " not found";
        BAD_STATUS(msg.str(), grpc::StatusCode::NOT_FOUND);
    }

    ParamAccessorData ans;
    catena::Param *p = device_.mutable_params()->at(oid).mutable_param();
    std::get<0>(ans) = p;
    std::get<1>(ans) = (p->has_value() ? p->mutable_value() : &noValue_);
    while (path_.size()) {
        ans = getSubparam_(path_, ans);
    }

    return ParamAccessor(*this, ans);
}

template <enum Threading T>
typename catena::DeviceModel<T>::ParamAccessorData
catena::DeviceModel<T>::getSubparam_(catena::Path &path, catena::DeviceModel<T>::ParamAccessorData &pad) {

    // destructure the param-value pair
    auto [parent, value] = pad;

    // validate the param type
    catena::ParamType_Type type = parent->type().type();
    switch (type) {
        case catena::ParamType_Type::ParamType_Type_STRUCT:
            // this is ok
            break;
        case catena::ParamType_Type::ParamType_Type_STRUCT_ARRAY:
            /** @todo implement sub-param navigation for STRUCT_ARRAY */
            BAD_STATUS("sub-param navigation for STRUCT_ARRAY not implemented, sorry",
                       grpc::StatusCode::UNIMPLEMENTED);
            break;
        default:
            std::stringstream err;
            err << "cannot sub-param param of type: " << type;
            BAD_STATUS((err.str()), grpc::StatusCode::INVALID_ARGUMENT);
    }

    // validate path argument and the parameter

    // is there a params field to define the sub-params?
    if (parent->params_size() == 0) {
        BAD_STATUS("params field is missing", grpc::StatusCode::FAILED_PRECONDITION);
    }

    // is our oid a string?
    auto seg = path.pop_front();
    if (!std::holds_alternative<std::string>(seg)) {
        BAD_STATUS("expected oid, got index", grpc::StatusCode::INVALID_ARGUMENT);
    }

    // is the oid defined?
    std::string oid(std::get<std::string>(seg));
    if (!parent->params().contains(oid)) {
        std::stringstream err;
        err << oid << " not found";
        BAD_STATUS((err.str()), grpc::StatusCode::FAILED_PRECONDITION);
    }

    // descend the value tree to find the value matching the oid
    catena::Value *v;
    if (value != &noValue_) {
        if (!(value->has_struct_value() || value->has_struct_array_values())) {
            BAD_STATUS("value must be of type struct_value or struct_array_value",
                       grpc::StatusCode::FAILED_PRECONDITION);
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
                v = &value->mutable_struct_value()->mutable_fields()->at(oid);
            }
        }

        if (value->has_struct_array_values()) {
            /** @todo implement value navigation for STRUCT_ARRAY */
            BAD_STATUS("value navigation for struct_array_value not implemented, sorry",
                       grpc::StatusCode::UNIMPLEMENTED);
        }
    }

    catena::Param *p = parent->mutable_params()->at(oid).mutable_param();
    return ParamAccessorData(p, v);
}

template <enum Threading T> std::ostream &operator<<(std::ostream &os, const catena::DeviceModel<T> &dm) {
    os << printJSON(dm.device());
    return os;
}

// template <enum Threading T>
// const std::string &catena::DeviceModel<T>::getOid(const Param &param) {
//   LockGuard lock(mutex_);
//   return param.param_.get().fqoid();
// }

// template <enum Threading T>
// catena::ParamAccessor
// catena::DeviceModel<T>::addParam(const std::string &jptr,
//                                  catena::Param &&param) {
//   catena::Path path(jptr);
//   LockGuard lock(mutex_);
//   if (path.size() > 1) {
//     /** @todo implement ability to add parameters below /params */
//     BAD_STATUS("implementation only supports adding params to top level",
//                grpc::StatusCode::UNIMPLEMENTED);
//   }
//   if (path.size() == 0) {
//     BAD_STATUS("empty path is invalid in this context",
//                grpc::StatusCode::INVALID_ARGUMENT);
//   }
//   catena::Param p(param);
//   auto seg = path.pop_front();
//   if (!std::holds_alternative<std::string>(seg)) {
//     std::stringstream err;
//     err << "invalid path: " << std::quoted(jptr);
//     BAD_STATUS(err.str(), grpc::StatusCode::INVALID_ARGUMENT);
//   }
//   std::string oid(std::get<std::string>(seg));

//   *(*device_.mutable_params())[oid].mutable_param() = p;
//   return Param(*this, *device_.mutable_params()->at(oid).mutable_param());
// }


// instantiate the 2 versions of DeviceModel, and its streaming operator
template class catena::DeviceModel<Threading::kMultiThreaded>;
template class catena::DeviceModel<Threading::kSingleThreaded>;

// instantiate the ostream operators
template std::ostream &operator<<(std::ostream &os, const catena::DeviceModel<Threading::kMultiThreaded> &dm);
template std::ostream &operator<<(std::ostream &os,
                                  const catena::DeviceModel<Threading::kSingleThreaded> &dm);
