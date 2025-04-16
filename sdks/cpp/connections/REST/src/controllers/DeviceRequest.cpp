
// connections/REST
#include <controllers/DeviceRequest.h>
using catena::REST::DeviceRequest;

// Initializes the object counter for Connect to 0.
int DeviceRequest::objectCounter_ = 0;

DeviceRequest::DeviceRequest(tcp::socket& socket, SocketReader& context, Device& dm) :
    socket_{socket}, writer_{socket, context.origin(), context.userAgent()}, context_{context}, dm_{dm}, ok_{true} {
    objectId_ = objectCounter_++;
    writeConsole(CallStatus::kCreate, socket_.is_open());
    // Parsing fields and assigning to respective variables.
    try {
        auto& dlMap = DetailLevel().getReverseMap(); // Reverse detail level map.
        if (dlMap.find(context_.fields("detail_level")) != dlMap.end()) {
            detailLevel_ = dlMap.at(context_.fields("detail_level"));
        } else {
            detailLevel_ = catena::Device_DetailLevel_NONE;
        }
    // Parse error
    } catch (...) {
        catena::exception_with_status err("Failed to parse fields", catena::StatusCode::INVALID_ARGUMENT);
        writer_.write(err);
        ok_ = false;
    }
}

void DeviceRequest::proceed() {
    if (!ok_) { return; }
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

void DeviceRequest::finish() {
    writeConsole(CallStatus::kFinish, socket_.is_open());
    writer_.finish();
    std::cout << "DeviceRequest[" << objectId_ << "] finished\n";
}
