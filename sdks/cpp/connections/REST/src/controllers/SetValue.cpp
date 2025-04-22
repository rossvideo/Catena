
// connections/REST
#include <controllers/SetValue.h>
using catena::REST::SetValue;

// Initializes the object counter for SetValue to 0.
int SetValue::objectCounter_ = 0;

SetValue::SetValue(tcp::socket& socket, SocketReader& context, IDevice* dm) :
    MultiSetValue(socket, context, dm, objectCounter_++) {
    writeConsole(CallStatus::kCreate, socket_.is_open());
}

bool SetValue::toMulti() {
    reqs_.set_slot(context_.slot());
    absl::Status status = google::protobuf::util::JsonStringToMessage(absl::string_view(context_.jsonBody()), reqs_.add_values());
    return status.ok();
}
