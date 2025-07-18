
// connections/REST
#include <controllers/Languages.h>
using catena::REST::Languages;

// Initializes the object counter for Languages to 0.
int Languages::objectCounter_ = 0;

Languages::Languages(tcp::socket& socket, ISocketReader& context, SlotMap& dms) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dms_{dms} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void Languages::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    // Getting language list from the device.
    catena::LanguageList ans;
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

        // GET/languages
        } else if (context_.method() == Method_GET) {
            std::lock_guard lg(dm->mutex());
            dm->toProto(ans);
            if (ans.languages().empty()) {
                rc = catena::exception_with_status("No languages found", catena::StatusCode::NOT_FOUND);
            }
        // Invalid method.
        } else {
            rc = catena::exception_with_status("", catena::StatusCode::UNIMPLEMENTED);
        }
    } catch (const catena::exception_with_status& err) {
        rc = catena::exception_with_status(std::string(err.what()), err.status);
    } catch (const std::exception& err) {
        rc = catena::exception_with_status(err.what(), catena::StatusCode::INTERNAL);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing answer to client.
    writer_.sendResponse(rc, ans);

    // Writing the final status to the console.
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    DEBUG_LOG << RESTMethodMap().getForwardMap().at(context_.method())
              << "Languages[" << objectId_ << "] finished";
}
