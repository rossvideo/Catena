
// connections/REST
#include <MultiSetValue.h>

// Initializes the object counter for MultiSetValue to 0.
int API::MultiSetValue::objectCounter_ = 0;

API::MultiSetValue::MultiSetValue(std::string& jsonPayload, tcp::socket& socket, Device& dm, catena::common::Authorizer* authz) :
    MultiSetValue(jsonPayload, socket, dm, authz, objectCounter_++) {
    typeName_ = "Multi";
    writeConsole(typeName_ + "SetValue", objectId_, CallStatus::kCreate, socket_.is_open());
    proceed();
    finish();
}

API::MultiSetValue::MultiSetValue(std::string& jsonPayload, tcp::socket& socket, Device& dm, catena::common::Authorizer* authz, int objectId) :
    jsonPayload_{jsonPayload}, socket_{socket}, writer_{socket}, dm_{dm}, authz_(authz), objectId_{objectId} {}

bool API::MultiSetValue::toMulti() {
    absl::Status status = google::protobuf::util::JsonStringToMessage(absl::string_view(jsonPayload_), &reqs_);
    return status.ok();
}

void API::MultiSetValue::proceed() {
    writeConsole(typeName_ + "SetValue", objectId_, CallStatus::kProcess, socket_.is_open());

    catena::exception_with_status rc{"", catena::StatusCode::OK};
    try {
        // Converting to MultiSetValuePayload.
        if (toMulti()) {
            // Trying and commiting the multiSetValue.
            {
            Device::LockGuard lg(dm_);
            if (dm_.tryMultiSetValue(reqs_, rc, *authz_)) {
                rc = dm_.commitMultiSetValue(reqs_, *authz_);
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
        writer_.write(ans);
    } else {
        writer_.write(rc);
    }
}

void API::MultiSetValue::finish() {
    writeConsole(typeName_ + "SetValue", objectId_, CallStatus::kFinish, socket_.is_open());
    std::cout << typeName_ << "SetValue[" << objectId_ << "] finished\n";
}
