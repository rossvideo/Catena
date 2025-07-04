
// connections/REST
#include <controllers/GetValue.h>
using catena::REST::GetValue;

// Initializes the object counter for GetValue to 0.
int GetValue::objectCounter_ = 0;

GetValue::GetValue(tcp::socket& socket, ISocketReader& context, SlotMap& dms) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dms_{dms} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void GetValue::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());
    catena::Value ans;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    try {
        IDevice* dm = nullptr;
        // Getting device at specified slot.
        if (dms_.contains(context_.slot())) {
            dm = dms_.at(context_.slot());
        }

        // Making sure the device exists.
        if (!dm) {
            rc = catena::exception_with_status("device not found in slot " + std::to_string(context_.slot()), catena::StatusCode::NOT_FOUND);

        // Getting value at oid from device.
        } else if (context_.authorizationEnabled()) {
            catena::common::Authorizer authz(context_.jwsToken());
            rc = dm->getValue(context_.fqoid(), ans, authz);
        } else {
            rc = dm->getValue(context_.fqoid(), ans, catena::common::Authorizer::kAuthzDisabled);
        }

    // ERROR
    } catch (const catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (const std::exception& err) {
        rc = catena::exception_with_status(err.what(), catena::StatusCode::INTERNAL);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing answer to client.
    writer_.sendResponse(rc, ans);

    // Writing the final status to the console.
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "GetValue[" << objectId_ << "] finished\n";
}
