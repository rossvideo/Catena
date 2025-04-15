
// connections/REST
#include <controllers/MultiSetValue.h>
using catena::REST::MultiSetValue;

// Initializes the object counter for MultiSetValue to 0.
int MultiSetValue::objectCounter_ = 0;

MultiSetValue::MultiSetValue(tcp::socket& socket, SocketReader& context, IDevice* dm) :
    MultiSetValue(socket, context, dm, objectCounter_++) {
    typeName_ = "Multi";
    writeConsole(CallStatus::kCreate, socket_.is_open());
}

MultiSetValue::MultiSetValue(tcp::socket& socket, SocketReader& context, IDevice* dm, int objectId) :
    socket_{socket}, writer_{socket}, context_{context}, dm_{dm}, objectId_{objectId} {}

bool MultiSetValue::toMulti() {
    absl::Status status = google::protobuf::util::JsonStringToMessage(absl::string_view(context_.jsonBody()), &reqs_);
    return status.ok();
}

void MultiSetValue::proceed() {
    writeConsole(CallStatus::kProcess, socket_.is_open());

    catena::exception_with_status rc{"", catena::StatusCode::OK};
    try {
        // Converting to MultiSetValuePayload.
        if (toMulti()) {
            // Setting up authorizer.
            std::shared_ptr<catena::common::Authorizer> sharedAuthz;
            catena::common::Authorizer* authz;
            if (context_.authorizationEnabled()) {
                sharedAuthz = std::make_shared<catena::common::Authorizer>(context_.jwsToken());
                authz = sharedAuthz.get();
            } else {
                authz = &catena::common::Authorizer::kAuthzDisabled;
            }
            // Trying and commiting the multiSetValue.
            {
            IDevice::LockGuard lg(dm_);
            if (dm_->tryMultiSetValue(reqs_, rc, *authz)) {
                rc = dm_->commitMultiSetValue(reqs_, *authz);
            }
            }
        } else {
            rc = catena::exception_with_status("Failed to convert JSON to protobuf", catena::StatusCode::INVALID_ARGUMENT);
        }
    // ERROR
    } catch (catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }
    // Writing response.
    if (rc.status == catena::StatusCode::OK) {
        catena::Empty ans = catena::Empty();
        writer_.finish(ans);
    } else {
        writer_.write(rc);
    }
}

void MultiSetValue::finish() {
    writeConsole(CallStatus::kFinish, socket_.is_open());
    std::cout << typeName_ << "SetValue[" << objectId_ << "] finished\n";
}
