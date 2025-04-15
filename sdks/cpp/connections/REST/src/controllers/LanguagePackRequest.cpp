
// connections/REST
#include <controllers/LanguagePackRequest.h>
using catena::REST::LanguagePackRequest;

// Initializes the object counter for LanguagePackRequest to 0.
int LanguagePackRequest::objectCounter_ = 0;

LanguagePackRequest::LanguagePackRequest(tcp::socket& socket, ISocketReader& context, Device& dm) :
    socket_{socket}, writer_{socket, context.origin()}, dm_{dm}, ok_{true} {
    objectId_ = objectCounter_++;
    writeConsole(CallStatus::kCreate, socket_.is_open());
    // Parsing fields and assigning to respective variables.
    try {
        std::unordered_map<std::string, std::string> fields = {
            {"language", ""},
            {"slot", ""}
        };
        context.fields(fields);
        slot_ = fields.at("slot") != "" ? std::stoi(fields.at("slot")) : 0;
        language_ = fields.at("language");
    // Parse error
    } catch (...) {
        catena::exception_with_status err("Failed to parse fields", catena::StatusCode::INVALID_ARGUMENT);
        writer_.write(err);
        ok_ = false;
    }
}

void LanguagePackRequest::proceed() {
    if (!ok_) { return; }
    writeConsole(CallStatus::kProcess, socket_.is_open());

    // Getting the requested language pack.
    catena::DeviceComponent_ComponentLanguagePack ans;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    try {
        Device::LockGuard lg(dm_);
        rc = dm_.getLanguagePack(language_, ans);
    // ERROR
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing answer to client.
    if (rc.status == catena::StatusCode::OK) {
        writer_.finish(ans);
    } else {
        writer_.write(rc);
    }
}

void LanguagePackRequest::finish() {
    writeConsole(CallStatus::kFinish, socket_.is_open());
    std::cout << "LanguagePackRequest[" << objectId_ << "] finished\n";
}
