
// connections/REST
#include <controllers/ListLanguages.h>
using catena::REST::ListLanguages;

// Initializes the object counter for ListLanguages to 0.
int ListLanguages::objectCounter_ = 0;

ListLanguages::ListLanguages(tcp::socket& socket, ISocketReader& context, Device& dm) :
    socket_{socket}, writer_{socket}, dm_{dm}, ok_{true} {
    objectId_ = objectCounter_++;
    writeConsole(CallStatus::kCreate, socket_.is_open());
    // Parsing fields and assigning to respective variables.
    try {
        std::unordered_map<std::string, std::string> fields = {
            {"slot", ""}
        };
        context.fields(fields);
        slot_ = fields.at("slot") != "" ? std::stoi(fields.at("slot")) : 0;
    // Parse error
    } catch (...) {
        catena::exception_with_status err("Failed to parse fields", catena::StatusCode::INVALID_ARGUMENT);
        writer_.write(err);
        ok_ = false;
    }
}

void ListLanguages::proceed() {
    if (!ok_) { return; }
    writeConsole(CallStatus::kProcess, socket_.is_open());

    // Getting language list from the device.
    catena::LanguageList ans;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    try {
        Device::LockGuard lg(dm_);
        dm_.toProto(ans);
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

void ListLanguages::finish() {
    writeConsole(CallStatus::kFinish, socket_.is_open());
    std::cout << "ListLanguages[" << objectId_ << "] finished\n";
}
