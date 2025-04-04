
// connections/REST
#include <DeviceRequest.h>

// Initializes the object counter for Connect to 0.
int API::DeviceRequest::objectCounter_ = 0;

API::DeviceRequest::DeviceRequest(tcp::socket& socket, SocketReader& context, Device& dm) :
    socket_{socket}, writer_{socket}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole(CallStatus::kCreate, socket_.is_open());
    // Return code used for status.
    catena::exception_with_status err("", catena::StatusCode::OK);
    // Parsing fields and assigning to respective variables.
    try {
        std::unordered_map<std::string, std::string> fields = {
            {"subscribed_oids", ""},
            {"detail_level", ""},
            {"language", ""},
            {"slot", ""}
        };
        context_.fields(fields);
        slot_ = fields.at("slot") != "" ? std::stoi(fields.at("slot")) : 0;
        language_ = fields.at("language");
        auto& dlMap = DetailLevel().getReverseMap(); // Reverse detail level map.
        if (dlMap.find(fields.at("detail_level")) != dlMap.end()) {
            detailLevel_ = dlMap.at(fields.at("detail_level"));
        } else {
            detailLevel_ = catena::Device_DetailLevel_NONE;
        }
        catena::split(subscribedOids_, fields.at("subscribed_oids"), ",");
    // Parse error
    } catch (...) {
        err = catena::exception_with_status("Failed to parse fields", catena::StatusCode::INVALID_ARGUMENT);
        writer_.write(err);
    }
    // If no issue above, continue to proceed.
    if (err.status == catena::StatusCode::OK) {
        proceed();
    }
    // Finish the RPC.
    finish();
}

void API::DeviceRequest::proceed() {
    writeConsole(CallStatus::kProcess, socket_.is_open());
    try {
        // controls whether shallow copy or deep copy is used
        bool shallowCopy = true;
        std::shared_ptr<catena::common::Authorizer> sharedAuthz;
        catena::common::Authorizer* authz;
        if (context_.authorizationEnabled()) {
            sharedAuthz = std::make_shared<catena::common::Authorizer>(context_.jwsToken());
            authz = sharedAuthz.get();
        } else {
            authz = &catena::common::Authorizer::kAuthzDisabled;
        }
        // Getting the component serializer.
        auto serializer = dm_.getComponentSerializer(*authz, shallowCopy);
        // Getting each component ans writing to the stream.
        while (serializer.hasMore()) {
            writeConsole(CallStatus::kWrite, socket_.is_open());
            catena::DeviceComponent component{};
            {
            Device::LockGuard lg(dm_);
            component = serializer.getNext();
            }
            writer_.write(component);
        }
    // ERROR: Write to stream and end call.
    } catch (catena::exception_with_status& err) {
        writer_.write(err);
    } catch (...) {
        catena::exception_with_status err{"Unknown errror", catena::StatusCode::UNKNOWN};
        writer_.write(err);
    }
}

void API::DeviceRequest::finish() {
    writeConsole(CallStatus::kFinish, socket_.is_open());
    writer_.finish();
    std::cout << "DeviceRequest[" << objectId_ << "] finished\n";
}
