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
using catena::DeviceStream;
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
void catena::DeviceModel::sendDevice(grpc::ServerAsyncWriter<::catena::DeviceComponent> *writer, void* tag) {
    std::lock_guard<Mutex> lock(mutex_);
    catena::DeviceComponent dc;
    dc.set_allocated_device(&device_);
    writer->Write(dc, tag);
    auto x = dc.release_device();
}

// for parameters that do not have values
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

    std::string scope = device_.default_scope();

    if (!device_.mutable_params()->contains(oid)) {
        std::stringstream msg;
        msg << "param " << std::quoted(oid) << " not found";
        BAD_STATUS(msg.str(), catena::StatusCode::NOT_FOUND);
    }

    ParamAccessorData pad;
    catena::Param &p = device_.mutable_params()->at(oid);
    std::get<0>(pad) = &p;
    std::get<1>(pad) = (p.has_value() ? p.mutable_value() : &noValue_);
    if (p.access_scope() != "") {
        scope = p.access_scope();
    }
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

DeviceStream::DeviceStream(catena::DeviceModel &dm) 
    : deviceModel_{dm}, component_{}, nextType_{ComponentType::BASIC_DEVICE_INFO}{
        const Device& device = deviceModel_.get().device();
        paramIter_ = device.params().begin();
        constraintIter_ = device.constraints().begin();
        menuGroupIter_ = device.menu_groups().begin();
        if (menuGroupIter_ != device.menu_groups().end()){
            menuIter_ = menuGroupIter_->second.menus().begin();
        }
        commandIter_ = device.commands().begin();
        languagePackIter_ = device.language_packs().packs().begin();
    }

void DeviceStream::attachClientScopes(std::vector<std::string>& scopes){
    clientScopes_ = &scopes;
}

bool DeviceStream::hasNext(){
    return nextType_ != ComponentType::FINISHED;
}

const catena::DeviceComponent& DeviceStream::next(){
    if (clientScopes_ == nullptr){
        throw std::runtime_error("Client scopes not attached");
    }
    switch(nextType_){
        case ComponentType::BASIC_DEVICE_INFO:
            return basicDeviceInfo_();
            break;

        case ComponentType::PARAM:
            return paramComponent_();
            break;

        case ComponentType::CONSTRAINT:
            return constraintComponent_();
            break;

        case ComponentType::MENU:
            return menuComponent_();
            break;

        case ComponentType::COMMAND:
            return commandComponent_();
            break;

        case ComponentType::LANGUAGE_PACK:
            return languagePackComponent_();
            break;
        case ComponentType::FINISHED:
            component_.Clear();
            return component_;
            break;
    }
    return component_;
}

void DeviceStream::setNextType_(){
    const Device& device = deviceModel_.get().device();
    // Skip over params that are not in the client's scope
    std::unique_ptr<ParamAccessor> p;
    while(paramIter_ != device.params().end()){
        p = deviceModel_.get().param("/" + paramIter_->first);
        if(p->checkScope(*clientScopes_) == true) {
            nextType_ = ComponentType::PARAM;
            return; 
        }
        paramIter_++;
    }
    if(constraintIter_ != device.constraints().end()){
        nextType_ = ComponentType::CONSTRAINT;
        return;
    }
    if(menuGroupIter_ != device.menu_groups().end()){
        nextType_ = ComponentType::MENU;
        return;
    }
    if(commandIter_ != device.commands().end()){
        nextType_ = ComponentType::COMMAND;
        return;
    }
    if(languagePackIter_ != device.language_packs().packs().end()){
        nextType_ = ComponentType::LANGUAGE_PACK;
        return;
    }
    nextType_ = ComponentType::FINISHED;
}

catena::DeviceComponent& DeviceStream::basicDeviceInfo_(){
    Device* basicInfo = component_.mutable_device();
    const Device& device = deviceModel_.get().device();
         
    basicInfo->set_slot(device.slot());
    basicInfo->set_detail_level(device.detail_level());
    basicInfo->set_multi_set_enabled(device.multi_set_enabled());
    basicInfo->set_subscriptions(device.subscriptions());
    for (const auto scope : device.access_scopes()){
    basicInfo->add_access_scopes(scope);
    }
    basicInfo->set_default_scope(device.default_scope());

    setNextType_();
    return component_;
}

catena::DeviceComponent& DeviceStream::paramComponent_(){
    catena::DeviceComponent_ComponentParam* param = component_.mutable_param();
    std::unique_ptr<ParamAccessor> p = deviceModel_.get().param("/" + paramIter_->first);
    try {
        p->getParam(param, *clientScopes_); // get the param
    } catch(catena::exception_with_status& why){
        // Error is thrown for clients without authorization
        // Don't need to send any info to unauthorized clients
    } 
  
    paramIter_++;
    setNextType_();
    return component_;
}

catena::DeviceComponent& DeviceStream::constraintComponent_(){
    catena::DeviceComponent_ComponentConstraint* constraint = component_.mutable_shared_constraint();
    constraint->set_oid(constraintIter_->first);
    *constraint->mutable_constraint() = constraintIter_->second;
    constraintIter_++;
    
    setNextType_();
    return component_;
}

catena::DeviceComponent& DeviceStream::menuComponent_(){
    catena::DeviceComponent_ComponentMenu* menu = component_.mutable_menu();
    menu->set_oid(menuIter_->first);
    *menu->mutable_menu() = menuIter_->second;
    menuIter_++;

    /**
     * @todo check if the menu corresponds to a param that is in the client's scope
    */
    if(menuIter_ == menuGroupIter_->second.menus().end()){
        menuGroupIter_++;
        if(menuGroupIter_ == deviceModel_.get().device().menu_groups().end()){
            setNextType_();
        }else{
            menuIter_ = menuGroupIter_->second.menus().begin();
        }
    }
    return component_;
}

catena::DeviceComponent& DeviceStream::commandComponent_(){
    catena::DeviceComponent_ComponentCommand* command = component_.mutable_command();
    command->set_oid(commandIter_->first);
    *command->mutable_param() = commandIter_->second;
    commandIter_++;
    
    setNextType_();
    return component_;
}

catena::DeviceComponent& DeviceStream::languagePackComponent_(){
    catena::DeviceComponent_ComponentLanguagePack* languagePack = component_.mutable_language_pack();
    languagePack->set_language(languagePackIter_->first);
    *languagePack->mutable_language_pack() = languagePackIter_->second;
    languagePackIter_++;
    
    setNextType_();
    return component_;
}

std::ostream &operator<<(std::ostream &os, const catena::DeviceModel &dm) {
    os << printJSON(dm.device());
    return os;
}