
// connections/REST
#include <controllers/SetValue.h>
using catena::REST::SetValue;

// Initializes the object counter for SetValue to 0.
int SetValue::objectCounter_ = 0;

SetValue::SetValue(ICatenaServiceImpl *service, tcp::socket& socket, ISocketReader& context, SlotMap& dms) :
    MultiSetValue(service, socket, context, dms, objectCounter_++) {
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

bool SetValue::toMulti_() {
    auto value = reqs_.add_values();
    reqs_.set_slot(context_.slot());
    absl::Status status = google::protobuf::util::JsonStringToMessage(absl::string_view(context_.jsonBody()), value->mutable_value());
    value->set_oid(context_.fqoid());
    return status.ok();
}
