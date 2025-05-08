
// connections/REST
#include <controllers/Connect.h>
using catena::REST::Connect;

// Initializes the object counter for Connect to 0.
int Connect::objectCounter_ = 0;

Connect::Connect(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, shutdown_{false}, context_{context},
    catena::common::Connect(dm, context.getSubscriptionManager()) {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
    // Parsing fields and assigning to respective variables.
    userAgent_ = context.fields("user_agent");
    forceConnection_ = context.hasField("force_connection");

    // Set detail level from context
    detailLevel_ = context_.detailLevel();
    dm_.detail_level(detailLevel_);
}

void Connect::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    try {
        // Setting up the client's authorizer.
        initAuthz_(context_.jwsToken(), context_.authorizationEnabled());
        // Cancels all open connections if shutdown signal is sent.
        shutdownSignalId_ = shutdownSignal_.connect([this](){
            shutdown_ = true;
            this->hasUpdate_ = true;
            this->cv_.notify_one();
        });
        // Waiting for a value set by server to be sent to execute code.
        valueSetByServerId_ = dm_.valueSetByServer.connect([this](const std::string& oid, const IParam* p, const int32_t idx){
            updateResponse_(oid, idx, p);
        });
        // Waiting for a value set by client to be sent to execute code.
        valueSetByClientId_ = dm_.valueSetByClient.connect([this](const std::string& oid, const IParam* p, const int32_t idx){
            updateResponse_(oid, idx, p);
        });
        // Waiting for a language to be added to execute code.
        languageAddedId_ = dm_.languageAddedPushUpdate.connect([this](const IDevice::ComponentLanguagePack& l) {
            updateResponse_(l);
        });
        // Waiting for a command to be executed to execute code.
        commandExecutedId_ = dm_.commandExecuted.connect([this](const std::string& oid, const IParam* command, const int32_t idx) {
            if (detailLevel_ == catena::Device_DetailLevel_COMMANDS) {
                hasCommandUpdate_ = true;
                updateResponse_(oid, idx, command);
            }
        });
        // send client an empty update with slot of the device
        catena::PushUpdates populatedSlots;
        populatedSlots.set_slot(dm_.slot());
        writer_.write(populatedSlots);
    // Used to catch the authz error.
    } catch (catena::exception_with_status& err) {
        writer_.finish(catena::Empty(), err);
        shutdown_ = true;
    }

    // kWrite: Waiting for updates to send to the client.
    std::unique_lock<std::mutex> lock{mtx_, std::defer_lock};
    while (socket_.is_open() && !shutdown_) {
        lock.lock();
        cv_.wait(lock, [this] { return hasUpdate_; });
        hasUpdate_ = false;
        writeConsole_(CallStatus::kWrite, true);
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
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    try {
        shutdownSignal_.disconnect(shutdownSignalId_);
        dm_.valueSetByClient.disconnect(valueSetByClientId_);
        dm_.valueSetByServer.disconnect(valueSetByServerId_);
        dm_.languageAddedPushUpdate.disconnect(languageAddedId_);
        dm_.commandExecuted.disconnect(commandExecutedId_);
    // Listener not yet initialized.
    } catch (...) {}
    // Finishing and closing the socket.
    if (socket_.is_open()) {
        writer_.finish(catena::Empty(), catena::exception_with_status("", catena::StatusCode::OK));
        socket_.close();
    }
    std::cout << "Connect[" << objectId_ << "] finished\n";
}
