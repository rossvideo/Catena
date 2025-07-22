// connections/REST
#include <controllers/Connect.h>
using catena::common::ILanguagePack;

// Initializes the object counter for Connect to 0.
int catena::REST::Connect::objectCounter_ = 0;

catena::REST::Connect::Connect(tcp::socket& socket, ISocketReader& context, SlotMap& dms) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context},
    catena::common::Connect(dms, context.subscriptionManager()) {
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

catena::REST::Connect::~Connect() {
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
    context_.service()->deregisterConnection(this);
}

void catena::REST::Connect::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    catena::exception_with_status rc{"", catena::StatusCode::OK};
    try {
        // Cancels all open connections if shutdown signal is sent.
        shutdownSignalId_ = shutdownSignal_.connect([this](){ shutdown(); });
        // Initialize variables and authz and add connection to the priority queue.
        detailLevel_ = context_.detailLevel();
        userAgent_ = context_.fields("user_agent");
        forceConnection_ = context_.hasField("force_connection");
        initAuthz_(context_.jwsToken(), context_.authorizationEnabled());
        if (context_.service()->registerConnection(this)) {
            // Connecting to each device in dms_.
            catena::PushUpdates populatedSlots;
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
        // Failed to register connection.
        } else {
            rc = catena::exception_with_status("Too many connections to service", catena::StatusCode::RESOURCE_EXHAUSTED);
            shutdown_ = true;
        }
    // Used to catch the authz error.
    } catch (catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (const std::exception& e) {
        rc = catena::exception_with_status(std::string("Connection setup failed: ") + e.what(), catena::StatusCode::INTERNAL);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }
    // If above failed, finish the RPC.
    if (rc.status != catena::StatusCode::OK) {
        shutdown_ = true;
        writer_.sendResponse(rc);
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

    // Writing the final status to the console.
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    DEBUG_LOG << "Connect[" << objectId_ << "] finished";
}
