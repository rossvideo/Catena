
// connections/REST
#include <controllers/AddLanguage.h>
using catena::REST::AddLanguage;

// Initializes the object counter for AddLanguage to 0.
int AddLanguage::objectCounter_ = 0;

AddLanguage::AddLanguage(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void AddLanguage::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    catena::exception_with_status rc("", catena::StatusCode::OK);
    try {
        // Constructing the AddLanguagePayload
        catena::AddLanguagePayload payload;
        payload.set_slot(context_.slot());
        payload.set_id(context_.fields("id"));
        absl::Status status = google::protobuf::util::JsonStringToMessage(absl::string_view(context_.jsonBody()), payload.mutable_language_pack());
        // Adding to device
        if (status.ok()) {
            if(context_.authorizationEnabled()) {
                catena::common::Authorizer authz{context_.jwsToken()};
                std::lock_guard lg(dm_.mutex());
                rc = dm_.addLanguage(payload, authz);
            } else {
                std::lock_guard lg(dm_.mutex());
                rc = dm_.addLanguage(payload, catena::common::Authorizer::kAuthzDisabled);
            }
        } else {
            rc = catena::exception_with_status("Failed to convert JSON to protobuf: " + status.ToString(), catena::StatusCode::INVALID_ARGUMENT);
        }
    // Likely authentication error, end process.
    } catch (catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (const std::exception& e) {
        rc = catena::exception_with_status(std::string("Error processing request: ") + e.what(), catena::StatusCode::INTERNAL);
    } catch (...) { // Error, end process.
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing answer to client.
    writer_.sendResponse(rc);
}

void AddLanguage::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "AddLanguage[" << objectId_ << "] finished\n";
}
