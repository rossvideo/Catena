// connections/REST
#include <controllers/Connect.h>
using catena::REST::Connect;
using catena::common::ILanguagePack;

// Initializes the object counter for Connect to 0.
int Connect::objectCounter_ = 0;

Connect::Connect(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, shutdown_{false}, context_{context},
    catena::common::Connect(dm, context.getSubscriptionManager()) {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
    
    try {
        // Parsing fields and assigning to respective variables.
        userAgent_ = context.fields("user_agent");
        forceConnection_ = context.hasField("force_connection");

        // Set detail level from context
        detailLevel_ = context_.detailLevel();
        dm_.detail_level(detailLevel_);
    } catch (const catena::exception_with_status& err) {
        throw; // Re-throw to preserve original status code
    } catch (const std::exception& e) {
        throw catena::exception_with_status(std::string("Failed to parse connection fields: ") + e.what(), 
                                          catena::StatusCode::INVALID_ARGUMENT);
    } catch (...) {
        throw catena::exception_with_status("Unknown error while parsing connection fields", 
                                          catena::StatusCode::UNKNOWN);
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
        // Waiting for a value set by server to be sent to execute code.
        valueSetByServerId_ = dm_.valueSetByServer.connect([this](const std::string& oid, const IParam* p, const int32_t idx){
            updateResponse_(oid, idx, p);
        });
        // Waiting for a value set by client to be sent to execute code.
        valueSetByClientId_ = dm_.valueSetByClient.connect([this](const std::string& oid, const IParam* p, const int32_t idx){
            updateResponse_(oid, idx, p);
        });
        // Waiting for a language to be added to execute code.
        languageAddedId_ = dm_.languageAddedPushUpdate.connect([this](const ILanguagePack* l) {
            updateResponse_(l);
        });
        // Send client an empty update with slot of the device
        catena::PushUpdates populatedSlots;
        populatedSlots.set_slot(dm_.slot());
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
        
        if (!socket_.is_open() || shutdown_) {
            lock.unlock();
            break;
        }

        try {
            res_.set_slot(dm_.slot());
            writer_.sendResponse(catena::exception_with_status("", catena::StatusCode::OK), res_);
        } catch (const std::exception& e) {
            // For errors, just send the status code without a response body
            try {
                writer_.sendResponse(catena::exception_with_status(
                    std::string("Failed to send update: ") + e.what(), 
                    catena::StatusCode::INTERNAL));
            } catch (...) {
                // If we can't send the error response, then the socket is truly dead
            }
            socket_.close();
            break;
        } catch (...) {
            // For errors, just send the status code without a response body
            try {
                writer_.sendResponse(catena::exception_with_status(
                    "Unknown error while sending update", 
                    catena::StatusCode::UNKNOWN));
            } catch (...) {
                // If we can't send the error response, then the socket is truly dead
            }
            socket_.close();
            break;
        }
        lock.unlock();
    }

    // If we get here, the connection is ending
    if (socket_.is_open()) {
        try {
            // Only try to send a final response if this was a clean shutdown
            if (shutdown_) {
                writer_.sendResponse(catena::exception_with_status("Connection closed by server", 
                                                                 catena::StatusCode::OK));
            }
        } catch (...) {
            // Ignore errors on final response - we're closing anyway
        }
        socket_.close();
    }
}

void Connect::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    try {
        shutdownSignal_.disconnect(shutdownSignalId_);
        dm_.valueSetByClient.disconnect(valueSetByClientId_);
        dm_.valueSetByServer.disconnect(valueSetByServerId_);
        dm_.languageAddedPushUpdate.disconnect(languageAddedId_);
    // Listener not yet initialized.
    } catch (...) {}
    
    // Only attempt to send response if socket is still open
    if (socket_.is_open()) {
        try {
            writer_.sendResponse(catena::exception_with_status("", catena::StatusCode::OK));
        } catch (...) {
            // If we can't send the response, just close the socket
        }
        socket_.close();
    }
    std::cout << "Connect[" << objectId_ << "] finished\n";
}
