
// connections/REST
#include <controllers/Connect.h>
using catena::REST::Connect;

// Initializes the object counter for Connect to 0.
int Connect::objectCounter_ = 0;

Connect::Connect(tcp::socket& socket, ISocketReader& context, Device& dm) :
    socket_{socket}, writer_{socket, context.origin(), context.userAgent()}, ok_{true}, shutdown_{false}, catena::common::Connect(dm, context.authorizationEnabled(), context.jwsToken()) {
    objectId_ = objectCounter_++;
    writeConsole(CallStatus::kCreate, socket_.is_open());
    // Parsing fields and assigning to respective variables.
    try {
        std::unordered_map<std::string, std::string> fields = {
            {"force_connection", ""},
            {"user_agent", ""},
            {"detail_level", ""},
            {"language", ""}
        };
        context.fields(fields);
        language_ = fields.at("language");
        auto& dlMap = DetailLevel().getReverseMap(); // Reverse detail level map.
        if (dlMap.find(fields.at("detail_level")) != dlMap.end()) {
            detailLevel_ = dlMap.at(fields.at("detail_level"));
        } else {
            detailLevel_ = catena::Device_DetailLevel_NONE;
        }
        userAgent_ = fields.at("user_agent");
        forceConnection_ = fields.at("force_connection") == "true";
    // Parse error
    } catch (...) {
        catena::exception_with_status err("Failed to parse fields", catena::StatusCode::INVALID_ARGUMENT);
        writer_.write(err);
        ok_ = false;
    }
}

void Connect::proceed() {
    if (!ok_) { return; }
    writeConsole(CallStatus::kProcess, socket_.is_open());
    // Cancels all open connections if shutdown signal is sent.
    shutdownSignalId_ = shutdownSignal_.connect([this](){
        shutdown_ = true;
        this->hasUpdate_ = true;
        this->cv_.notify_one();
    });
    // Waiting for a value set by server to be sent to execute code.
    valueSetByServerId_ = dm_.valueSetByServer.connect([this](const std::string& oid, const IParam* p, const int32_t idx){
        updateResponse(oid, idx, p);
    });
    // Waiting for a value set by client to be sent to execute code.
    valueSetByClientId_ = dm_.valueSetByClient.connect([this](const std::string& oid, const IParam* p, const int32_t idx){
        updateResponse(oid, idx, p);
    });
    // Waiting for a language to be added to execute code.
    languageAddedId_ = dm_.languageAddedPushUpdate.connect([this](const Device::ComponentLanguagePack& l) {
        updateResponse(l);
    });
    // send client an empty update with slot of the device
    catena::PushUpdates populatedSlots;
    populatedSlots.set_slot(dm_.slot());
    writer_.write(populatedSlots);

    // kWrite: Waiting for updates to send to the client.
    std::unique_lock<std::mutex> lock{mtx_, std::defer_lock};
    while (socket_.is_open() && !shutdown_) {
        lock.lock();
        cv_.wait(lock, [this] { return hasUpdate_; });
        hasUpdate_ = false;
        writeConsole(CallStatus::kWrite, true);
        try {
            if (socket_.is_open() && !shutdown_) {
                res_.set_slot(dm_.slot());
                writer_.write(res_);
            }
        // A little scuffed but I have no idea how else to detect disconnect.
        } catch (...) {
            socket_.close();
        }
        lock.unlock();
    }
}

void Connect::finish() {
    writeConsole(CallStatus::kFinish, socket_.is_open());
    try {
        shutdownSignal_.disconnect(shutdownSignalId_);
        dm_.valueSetByClient.disconnect(valueSetByClientId_);
        dm_.valueSetByServer.disconnect(valueSetByServerId_);
        dm_.languageAddedPushUpdate.disconnect(languageAddedId_);
    // Listener not yet initialized.
    } catch (...) {}
    // Finishing and closing the socket.
    if (socket_.is_open()) {
        writer_.finish();
        socket_.close();
    }
    std::cout << "Connect[" << objectId_ << "] finished\n";
}
