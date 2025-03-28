
// connections/REST
#include <SetValue.h>

// Initializes the object counter for SetValue to 0.
int API::SetValue::objectCounter_ = 0;

API::SetValue::SetValue(std::string& jsonPayload, tcp::socket& socket, Device& dm, catena::common::Authorizer* authz) :
    MultiSetValue(jsonPayload, socket, dm, authz, objectCounter_++) {
    writeConsole("SetValue", objectId_, CallStatus::kCreate, socket_.is_open());
    proceed();
    finish();
}

bool API::SetValue::toMulti() {
    catena::SingleSetValuePayload payload;
    absl::Status status = google::protobuf::util::JsonStringToMessage(absl::string_view(jsonPayload_), &payload);
    if (status.ok()) {
        reqs_.set_slot(payload.slot());
        reqs_.add_values()->CopyFrom(payload.value());
    }
    return status.ok();
}
