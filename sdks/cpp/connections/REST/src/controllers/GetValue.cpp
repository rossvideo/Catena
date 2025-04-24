
// connections/REST
#include <controllers/GetValue.h>
using catena::REST::GetValue;

// Initializes the object counter for GetValue to 0.
int GetValue::objectCounter_ = 0;

GetValue::GetValue(tcp::socket& socket, SocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole(CallStatus::kCreate, socket_.is_open());
}

void GetValue::proceed() {
    writeConsole(CallStatus::kProcess, socket_.is_open());
    catena::Value ans;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    try {
        // Getting value at oid from device.
        if (context_.authorizationEnabled()) {
            catena::common::Authorizer authz(context_.jwsToken());
            rc = dm_.getValue("/" + context_.fields("oid"), ans, authz);
        } else {
            rc = dm_.getValue("/" + context_.fields("oid"), ans, catena::common::Authorizer::kAuthzDisabled);
        }

    // ERROR
    } catch (const catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
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

void GetValue::finish() {
    writeConsole(CallStatus::kFinish, socket_.is_open());
    std::cout << "GetValue[" << objectId_ << "] finished\n";
}
