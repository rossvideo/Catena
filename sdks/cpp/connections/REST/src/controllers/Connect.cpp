// connections/REST
#include <controllers/Connect.h>
using catena::REST::Connect;
using catena::common::ILanguagePack;

// Initializes the object counter for Connect to 0.
int Connect::objectCounter_ = 0;

Connect::Connect(tcp::socket& socket, ISocketReader& context, SlotMap& dms) :
    socket_{socket}, writer_{socket, context.origin()}, shutdown_{false}, context_{context},
    catena::common::Connect(dms, context.getSubscriptionManager()) {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
    
    // Parsing fields and assigning to respective variables.
    userAgent_ = context.fields("user_agent");
    forceConnection_ = context.hasField("force_connection");
}

Connect::~Connect() {
    // Disconnecting all initialized listeners.
    if (shutdownSignalId_ != 0) { shutdownSignal_.disconnect(shutdownSignalId_); }
    for (auto [slot, dm] : dms_) {
        if (dm) {
            if (valueSetByClientIds_.contains(slot)) {
                dm->getValueSetByClient().disconnect(valueSetByClientIds_[slot]);
            }
            if (valueSetByServerIds_.contains(slot)) {
                dm->getValueSetByServer().disconnect(valueSetByServerIds_[slot]);
            }
            if (languageAddedIds_.contains(slot)) {
                dm->getLanguageAddedPushUpdate().disconnect(languageAddedIds_[slot]);
            }
        }
    }
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
        // Set detail level from request
        detailLevel_ = context_.detailLevel();

        catena::PushUpdates populatedSlots;

        // Connecting to each device in dms_.
        for (auto [slot, dm] : dms_) {
            if (dm) {
                // Waiting for a value set by server to be sent to execute code.
                valueSetByServerIds_[slot] = dm->getValueSetByServer().connect([this, slot](const std::string& oid, const IParam* p){
                    updateResponse_(oid, p, slot);
                });
                // Waiting for a value set by client to be sent to execute code.
                valueSetByClientIds_[slot] = dm->getValueSetByClient().connect([this, slot](const std::string& oid, const IParam* p){
                    updateResponse_(oid, p, slot);
                });
                // Waiting for a language to be added to execute code.
                languageAddedIds_[slot] = dm->getLanguageAddedPushUpdate().connect([this, slot](const ILanguagePack* l) {
                    updateResponse_(l, slot);
                });
                populatedSlots.mutable_slots_added()->add_slots(slot);
            }
        }
        // Send client a empty update with slots populated by devices.
        writer_.sendResponse(catena::exception_with_status("", catena::StatusCode::OK), populatedSlots); 
    // Used to catch the authz error.
    } catch (catena::exception_with_status& err) {
        writer_.sendResponse(err);
        shutdown_ = true;
    } catch (const std::exception& e) {
        writer_.sendResponse(catena::exception_with_status(std::string("Connection setup failed: ") + e.what(), 
                                                         catena::StatusCode::INTERNAL));
        shutdown_ = true;
    } catch (...) {
        writer_.sendResponse(catena::exception_with_status("Unknown error during connection setup", 
                                                         catena::StatusCode::UNKNOWN));
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
                writer_.sendResponse(catena::exception_with_status("", catena::StatusCode::OK), res_);
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
    DEBUG_LOG << "Connect[" << objectId_ << "] finished";
}
