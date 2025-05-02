
// connections/REST
#include <controllers/LanguagePackRequest.h>
using catena::REST::LanguagePackRequest;

// Initializes the object counter for LanguagePackRequest to 0.
int LanguagePackRequest::objectCounter_ = 0;

LanguagePackRequest::LanguagePackRequest(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void LanguagePackRequest::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    // Getting the requested language pack.
    catena::DeviceComponent_ComponentLanguagePack ans;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    try {
        std::lock_guard lg(dm_.mutex());
        rc = dm_.getLanguagePack(context_.fields("language"), ans);
    // ERROR
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing answer to client.
    if (rc.status == catena::StatusCode::OK) {
        writer_.write(ans);
    } else {
        writer_.finish(rc);
    }
}

void LanguagePackRequest::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "LanguagePackRequest[" << objectId_ << "] finished\n";
}
