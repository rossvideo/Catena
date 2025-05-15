
// connections/REST
#include <controllers/ListLanguages.h>
using catena::REST::ListLanguages;

// Initializes the object counter for ListLanguages to 0.
int ListLanguages::objectCounter_ = 0;

ListLanguages::ListLanguages(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void ListLanguages::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    // Getting language list from the device.
    catena::LanguageList ans;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    try {
        std::lock_guard lg(dm_.mutex());
        dm_.toProto(ans);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing answer to client.
    writer_.sendResponse(rc, ans);
}

void ListLanguages::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "ListLanguages[" << objectId_ << "] finished\n";
}
