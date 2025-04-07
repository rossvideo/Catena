
// connections/REST
#include <controllers/SetValue.h>

// Initializes the object counter for SetValue to 0.
int CatenaServiceImpl::SetValue::objectCounter_ = 0;

CatenaServiceImpl::SetValue::SetValue(tcp::socket& socket, SocketReader& context, Device& dm) :
    MultiSetValue(socket, context, dm, objectCounter_++) {
    writeConsole(CallStatus::kCreate, socket_.is_open());
    proceed();
    finish();
}

bool CatenaServiceImpl::SetValue::toMulti() {
    catena::SingleSetValuePayload payload;
    absl::Status status = google::protobuf::util::JsonStringToMessage(absl::string_view(context_.jsonBody()), &payload);
    if (status.ok()) {
        reqs_.set_slot(payload.slot());
        reqs_.add_values()->CopyFrom(payload.value());
    }
    return status.ok();
}
