
// connections/REST
#include <controllers/AddLanguage.h>
using catena::REST::AddLanguage;

// Initializes the object counter for AddLanguage to 0.
int AddLanguage::objectCounter_ = 0;

AddLanguage::AddLanguage(tcp::socket& socket, SocketReader& context, Device& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm}, ok_{true} {
    objectId_ = objectCounter_++;
    writeConsole(CallStatus::kCreate, socket_.is_open());
    // Parsing fields and assigning to respective variables.
    try {
        std::unordered_map<std::string, std::string> fields = {
            {"id", ""},
            {"slot", ""}
        };
        context.fields(fields);
        slot_ = fields.at("slot") != "" ? std::stoi(fields.at("slot")) : 0;
        id_ = fields.at("id");
    // Parse error
    } catch (...) {
        catena::exception_with_status err("Failed to parse fields", catena::StatusCode::INVALID_ARGUMENT);
        writer_.write(err);
        ok_ = false;
    }
}

void AddLanguage::proceed() {
    if (!ok_) { return; }
    writeConsole(CallStatus::kProcess, socket_.is_open());

    catena::exception_with_status rc("", catena::StatusCode::OK);
    try {
        // Constructing the AddLanguagePayload
        catena::AddLanguagePayload payload;
        payload.set_slot(slot_);
        payload.set_id(id_);
        absl::Status status = google::protobuf::util::JsonStringToMessage(absl::string_view(context_.jsonBody()), payload.mutable_language_pack());
        // Adding to device
        if (status.ok()) {
            if(context_.authorizationEnabled()) {
                catena::common::Authorizer authz{context_.jwsToken()};
                Device::LockGuard lg(dm_);
                rc = dm_.addLanguage(payload, authz);
            } else {
                Device::LockGuard lg(dm_);
                rc = dm_.addLanguage(payload, catena::common::Authorizer::kAuthzDisabled);
            }
        } else {
            rc = catena::exception_with_status("Failed to convert JSON to protobuf", catena::StatusCode::INVALID_ARGUMENT);
        }
    // Likely authentication error, end process.
    } catch (catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (...) { // Error, end process.
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing answer to client.
    if (rc.status == catena::StatusCode::OK) {
        catena::Empty ans = catena::Empty();
        writer_.finish(ans);
    } else {
        writer_.write(rc);
    }
}

void AddLanguage::finish() {
    writeConsole(CallStatus::kFinish, socket_.is_open());
    std::cout << "AddLanguage[" << objectId_ << "] finished\n";
}
