
// connections/REST
#include <controllers/MultiSetValue.h>
using catena::REST::MultiSetValue;

// Initializes the object counter for MultiSetValue to 0.
int MultiSetValue::objectCounter_ = 0;

MultiSetValue::MultiSetValue(tcp::socket& socket, ISocketReader& context, SlotMap& dms) :
    MultiSetValue(socket, context, dms, objectCounter_++) {
    typeName_ = "Multi";
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

MultiSetValue::MultiSetValue(tcp::socket& socket, ISocketReader& context, SlotMap& dms, int objectId) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dms_{dms}, objectId_{objectId} {}

bool MultiSetValue::toMulti_() {
    absl::Status status = google::protobuf::util::JsonStringToMessage(absl::string_view(context_.jsonBody()), &reqs_);
    reqs_.set_slot(context_.slot());
    return status.ok();
}

void MultiSetValue::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    catena::exception_with_status rc{"", catena::StatusCode::OK};
    try {
        IDevice* dm = nullptr;
        // Validating slot number.
        if (context_.slot() < 0 || context_.slot() > 65535) {
            throw catena::exception_with_status("slot number out of range", catena::StatusCode::INVALID_ARGUMENT);
        }

        // Getting device at specified slot.
        if (dms_.contains(context_.slot())) {
            dm = dms_.at(context_.slot());
        }

        // Making sure the device exists.
        if (!dm) {
            rc = catena::exception_with_status("device not found in slot " + std::to_string(context_.slot()), catena::StatusCode::NOT_FOUND);

        // Converting to MultiSetValuePayload.
        } else if (toMulti_()) {
            // Setting up authorizer.
            std::shared_ptr<catena::common::Authorizer> sharedAuthz;
            catena::common::Authorizer* authz;
            if (context_.authorizationEnabled()) {
                sharedAuthz = std::make_shared<catena::common::Authorizer>(context_.jwsToken());
                authz = sharedAuthz.get();
            } else {
                authz = &catena::common::Authorizer::kAuthzDisabled;
            }
            // Trying and commiting the multiSetValue.
            {
            std::lock_guard lg(dm->mutex());
            if (dm->tryMultiSetValue(reqs_, rc, *authz)) {
                rc = dm->commitMultiSetValue(reqs_, *authz);
            } else { // debug log (new)
                LOG(ERROR) << "MultiSetValue: tryMultiSetValue failed for slot " << reqs_.slot()
                        << " status=" << static_cast<int>(rc.status)
                        << " msg=\"" << rc.what() << "\"";
            }
            }
        } else {
            rc = catena::exception_with_status("Failed to convert JSON to protobuf", catena::StatusCode::INVALID_ARGUMENT);
        }
    // ERROR
    } catch (catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }
    // Writing response.
    writer_.sendResponse(rc);

    // Writing the final status to the console.
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    LOG(INFO) << RESTMethodMap().getForwardMap().at(context_.method())
            << typeName_ << "SetValue[" << objectId_ << "] finished\n";
}
