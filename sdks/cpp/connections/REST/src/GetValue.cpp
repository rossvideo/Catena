
// connections/REST
#include <GetValue.h>

// Initializes the object counter for GetValue to 0.
int API::GetValue::objectCounter_ = 0;

API::GetValue::GetValue(std::string& request, tcp::socket& socket, Device& dm, catena::common::Authorizer* authz) :
    socket_{socket}, writer_{socket}, dm_{dm}, authz_{authz} {
    objectId_ = objectCounter_++;
    writeConsole("GetValue", objectId_, CallStatus::kCreate, socket_.is_open());
    // Return code used for status.
    catena::exception_with_status err("", catena::StatusCode::OK);
    // Parsing fields and assigning to respective variables.
    try {
        std::unordered_map<std::string, std::string> fields = {
            {"oid", ""},
            {"slot", ""}
        };
        parseFields(request, fields);
        slot_ = fields.at("slot") != "" ? std::stoi(fields.at("slot")) : 0;
        oid_ = fields.at("oid");
    // Parse error
    } catch (...) {
        err = catena::exception_with_status("Failed to parse fields", catena::StatusCode::INVALID_ARGUMENT);
        writer_.write(err);
    }
    // If no issue above, continue to proceed.
    if (err.status == catena::StatusCode::OK) {
        proceed();
    }
    // Finish the RPC.
    finish();
}

void API::GetValue::proceed() {
    writeConsole("GetValue", objectId_, CallStatus::kProcess, socket_.is_open());
    catena::Value ans;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    try {
        // Getting value at oid from device.
        rc = dm_.getValue(oid_, ans, *authz_);

    // ERROR
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing answer to client.
    if (rc.status == catena::StatusCode::OK) {
        writer_.write(ans);
    } else {
        writer_.write(rc);
    }
}

void API::GetValue::finish() {
    writeConsole("GetValue", objectId_, CallStatus::kFinish, socket_.is_open());
    std::cout << "GetValue[" << objectId_ << "] finished\n";
}
