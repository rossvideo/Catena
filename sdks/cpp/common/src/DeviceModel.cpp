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

// JSON parse options, used by a few methods defined here
auto jpopts = google::protobuf::util::JsonParseOptions{};


DeviceModel::DeviceModel(const std::string &filename) : device_{} {
    // initialize static attributes
    noValue_.set_undefined_value(catena::UndefinedValue());

    // read in the top level file, code block used to minimize
    // lifespan of the imported file
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
    current_folder = current_folder.remove_filename() /= std::string("params");
    importSubParams_(current_folder, *device_.mutable_params());
}

void DeviceModel::importSubParams_(std::filesystem::path &current_folder, ParamsMap &params) {
    for (auto it = params.begin(); it != params.end(); ++it) {
        catena::Param &child = params.at(it->first);
        if (child.has_import()) {
            if (child.import().url().compare("include") == 0) {
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
                child.clear_import();
                auto status = google::protobuf::util::JsonStringToMessage(imported, &child, jpopts);
                if (!status.ok()) {
                    std::stringstream err;
                    err << "error importing " << to_import << ": " << status.message();
                    BAD_STATUS(err.str(), catena::StatusCode::INVALID_ARGUMENT);
                }

                // recurse to import any sub-params
                if (child.params_size() > 0) {
                    std::filesystem::path next_folder(current_folder);
                    next_folder = next_folder /= it->first;
                    importSubParams_(next_folder, *child.mutable_params());
                }
            } else if (child.import().url().compare("") != 0) {
                /** @todo implement url imports*/
                BAD_STATUS("Cannot (yet) import from urls, sorry.", catena::StatusCode::UNIMPLEMENTED);
            }
        }
    }
}

const catena::Device &catena::DeviceModel::device() const {
    std::lock_guard<Mutex> lock(mutex_);
    return device_;
}

// send device info to client via writer
bool catena::DeviceModel::streamDevice(grpc::ServerAsyncWriter<::catena::DeviceComponent> *writer, void* tag) {
    std::lock_guard<Mutex> lock(mutex_);
    catena::DeviceComponent dc;
    dc.set_allocated_device(&device_);
    writer->Write(dc, tag);
    auto x = dc.release_device();
    return true; // we're sending the whole device for now, so signal that we're done
}

// for parameters that do not have the corresponding fields
catena::Value catena::DeviceModel::noValue_;

std::unique_ptr<ParamAccessor> catena::DeviceModel::param(const std::string &jptr) {
    std::lock_guard<Mutex> lock(mutex_);
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

    // build the data needed to create the ParamAccessor
    catena::Param &p = device_.mutable_params()->at(oid);
    // look for missing fields in the template
    checkTemplateData_(p, p.template_oid());
    if (p.params_size() > 0) {
        for (auto &[child_oid, child_param] : *p.mutable_params()) {
            checkTemplateData_(child_param, child_param.template_oid());
            // update the parent's value
            catena::StructField field;
            field.mutable_value()->CopyFrom(child_param.value());
            p.mutable_value()->mutable_struct_value()->mutable_fields()->insert({child_oid, field});
        }
    }
    ParamAccessorData pad;
    std::get<0>(pad) = &p;
    std::get<1>(pad) = (p.has_value() ? p.mutable_value() : &noValue_);
    std::string scope = p.access_scope() == "" ? device_.default_scope() : p.access_scope();

    auto ans = std::make_unique<ParamAccessor>(*this, pad, jptr, scope);

    while (path_.size()) {
        if (std::holds_alternative<std::string>(path_.front())) {
            std::string oid(std::get<std::string>(path_.pop_front()));
            ans = ans->subParam<false>(oid);
        } else if (std::holds_alternative<std::deque<std::string>::size_type>(path_.front())) {
            BAD_STATUS("indexing not yet implemented", catena::StatusCode::UNIMPLEMENTED);
            // auto idx = std::get<std::deque<std::string>::size_type>(path_.pop_front());
            // ans = ans->at<false>(idx);
        } else {
            BAD_STATUS("expected oid or index", catena::StatusCode::INVALID_ARGUMENT);
        }
    }

    return ans;
}

void DeviceModel::checkTemplateData_(catena::Param &p, const std::string &path) {
    if (path.empty()) { return; }

    catena::Path template_path_(path);
    catena::Path::Segment segment = template_path_.pop_front();
    if (!std::holds_alternative<std::string>(segment)) {
        BAD_STATUS("expected template_oid, got an index", catena::StatusCode::INVALID_ARGUMENT);
    }
    std::string toid(std::get<std::string>(segment));

    if (!device_.mutable_params()->contains(toid)) {
        std::stringstream msg;
        msg << "template " << std::quoted(toid) << " not found";
        BAD_STATUS(msg.str(), catena::StatusCode::NOT_FOUND);
    }
    
    // follow the template_oid as far as it goes
    std::reference_wrapper<catena::Param> t = std::ref(device_.mutable_params()->at(toid));
    while (template_path_.size()) {
        if (std::holds_alternative<std::string>(template_path_.front())) {
            if (t.get().value().kind_case() != Value::KindCase::kStructValue && 
                t.get().value().kind_case() != Value::KindCase::kStructVariantValue) {
                BAD_STATUS("cannot subparam into non-struct or variant type for template_oid", catena::StatusCode::INVALID_ARGUMENT);
            }
            std::string toid(std::get<std::string>(template_path_.pop_front()));
            t = std::ref(t.get().mutable_params()->at(toid));
        } else if (std::holds_alternative<std::deque<std::string>::size_type>(template_path_.front())) {
            // rebind t to the indexed param from the array when implementing indexing
            BAD_STATUS("indexing into template_oid not yet implemented", catena::StatusCode::UNIMPLEMENTED);
        } else {
            BAD_STATUS("expected oid or index in template_oid", catena::StatusCode::INVALID_ARGUMENT);
        }
    }

    // deep copy data into missing fields if this template has them
    // simple typed fields
    if (t.get().read_only()) {
        p.set_read_only(t.get().read_only());
    }
    if (t.get().response()) {
        p.set_response(t.get().response());
    }
    if (t.get().minimal_set()) {
        p.set_minimal_set(t.get().minimal_set());
    }
    if (t.get().stateless()) {
        p.set_stateless(t.get().stateless());
    }
    if (p.precision() == 0 && t.get().precision() != 0) {
        p.set_precision(t.get().precision());
    }  
    if (p.max_length() == 0 && t.get().max_length() != 0) {
        p.set_max_length(t.get().max_length());
    }
    if (p.widget().empty() && !t.get().widget().empty()) {
        p.set_widget(t.get().widget());
    }
    if (p.access_scope().empty() && !t.get().access_scope().empty()) {
        p.set_access_scope(t.get().access_scope());
    }
    if (p.type() == catena::ParamType::UNDEFINED && t.get().type() != catena::ParamType::UNDEFINED) {
        p.set_type(t.get().type());
    }
    // message fields
    if (!p.has_name() && t.get().has_name()) {
        *p.mutable_name() = t.get().name();
    }
    if (!p.has_value() && t.get().has_value()) {
        *p.mutable_value() = t.get().value();
    }
    if (!p.has_constraint() && t.get().has_constraint()) {
        *p.mutable_constraint() = t.get().constraint();
    }
    if (!p.has_help() && t.get().has_help()) {
        *p.mutable_help() = t.get().help();
    }
    if (!p.has_import() && t.get().has_import()) {
        *p.mutable_import() = t.get().import();
    }
    // fields of data structures
    if (p.params().empty() && !t.get().params().empty()) {
        *p.mutable_params() = t.get().params();
    }
    if (p.client_hints().empty() && !t.get().client_hints().empty()) {
        *p.mutable_client_hints() = t.get().client_hints();
    }
    if (p.commands().empty() && !t.get().commands().empty()) {
        *p.mutable_commands() = t.get().commands();
    }
    if (p.oid_aliases().empty() && !t.get().oid_aliases().empty()) {
        *p.mutable_oid_aliases() = t.get().oid_aliases();
    }
    
    // go deeper if this template has a template_oid
    checkTemplateData_(p, t.get().template_oid());
}

std::ostream &operator<<(std::ostream &os, const catena::DeviceModel &dm) {
    os << printJSON(dm.device());
    return os;
}