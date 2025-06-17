
// connections/REST
#include <controllers/Languages.h>
using catena::REST::Languages;

// Initializes the object counter for Languages to 0.
int Languages::objectCounter_ = 0;

Languages::Languages(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void Languages::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    // Getting language list from the device.
    catena::LanguageList ans;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    try {
        // GET/languages
        if (context_.method() == "GET") {
            std::lock_guard lg(dm_.mutex());
            dm_.toProto(ans);
            if (ans.languages().empty()) {
                rc = catena::exception_with_status("No languages found", catena::StatusCode::NOT_FOUND);
            }
        // Invalid method.
        } else {
            rc = catena::exception_with_status("", catena::StatusCode::INVALID_ARGUMENT);
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
}

void Languages::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "Languages[" << objectId_ << "] finished\n";
}
