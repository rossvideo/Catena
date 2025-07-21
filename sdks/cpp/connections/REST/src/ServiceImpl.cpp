// connections/REST
#include <ServiceImpl.h>
using catena::REST::CatenaServiceImpl;

// REST controllers
#include <controllers/Connect.h>
#include <controllers/GetParam.h>
#include <controllers/MultiSetValue.h>
#include <controllers/SetValue.h>
#include <controllers/DeviceRequest.h>
#include <controllers/AssetRequest.h>
#include <controllers/GetValue.h>
#include <controllers/GetPopulatedSlots.h>
#include <controllers/LanguagePack.h>
#include <controllers/Languages.h>
#include <controllers/ParamInfoRequest.h>
#include <controllers/Subscriptions.h>
#include <controllers/ExecuteCommand.h>

using catena::REST::Connect;

// Defining the port flag from SharedFlags.h
ABSL_FLAG(uint16_t, port, 443, "Catena REST service port");

// (UNUSED) expand env variables
// GCOVR_EXCL_START
void expandEnvVariables(std::string &str) {
    static std::regex env{"\\$\\{([^}]+)\\}"};
    std::smatch match;
    while (std::regex_search(str, match, env)) {
        const char *s = getenv(match[1].str().c_str());
        const std::string var(s == nullptr ? "" : s);
        str.replace(match[0].first, match[0].second, var);
    }
}
// GCOVR_EXCL_STOP

CatenaServiceImpl::CatenaServiceImpl(std::vector<IDevice*> dms, std::string& EOPath, bool authz, uint16_t port)
    : version_{"v1"},
      EOPath_{EOPath},
      port_{port},
      authorizationEnabled_{authz},
      acceptor_{io_context_, tcp::endpoint(tcp::v4(), port)},
      router_{Router::getInstance()} {

    if (authorizationEnabled_) { DEBUG_LOG <<"Authorization enabled."; }
    // Adding dms to slotMap.
    for (auto dm : dms) {
        if (dms_.contains(dm->slot())) {
            throw std::runtime_error("Device with slot " + std::to_string(dm->slot()) + " already exists in the map.");
        } else {
            dms_[dm->slot()] = dm;
        }
    }

    // Initializing the routes for router_ unless already done.
    if (!router_.canMake("PUT/subscriptions")) {
        router_.addProduct("GET/connect",         Connect::makeOne);
        router_.addProduct("GET",                 DeviceRequest::makeOne);
        router_.addProduct("POST/command",        ExecuteCommand::makeOne);
        router_.addProduct("GET/asset",           AssetRequest::makeOne);
        router_.addProduct("POST/asset",          AssetRequest::makeOne);
        router_.addProduct("GET/devices",         GetPopulatedSlots::makeOne);
        router_.addProduct("GET/value",           GetValue::makeOne);
        router_.addProduct("PUT/values",          MultiSetValue::makeOne);
        router_.addProduct("PUT/value",           SetValue::makeOne);
        router_.addProduct("GET/param",           GetParam::makeOne);
        router_.addProduct("GET/language-pack",   LanguagePack::makeOne);
        router_.addProduct("PUT/language-pack",   LanguagePack::makeOne);
        router_.addProduct("POST/language-pack",  LanguagePack::makeOne);
        router_.addProduct("DELETE/language-pack", LanguagePack::makeOne);
        router_.addProduct("GET/languages",       Languages::makeOne);
        router_.addProduct("GET/param-info",      ParamInfoRequest::makeOne);
        router_.addProduct("GET/subscriptions",   Subscriptions::makeOne);
        router_.addProduct("PUT/subscriptions",   Subscriptions::makeOne);
    }
}

// Initializing the shutdown signal for all open connections.
vdk::signal<void()> catena::REST::Connect::shutdownSignal_;

void CatenaServiceImpl::run() {
    // TLS handled by Envoyproxy
    shutdown_ = false;

    while (!shutdown_) {
        // Waiting for a connection.
        tcp::socket socket(io_context_);
        acceptor_.accept(socket);
        // Once a connection is made, increment activeRequests and handle async.
        {
            std::lock_guard<std::mutex> lock(activeRequestMutex_);
            activeRequests_ += 1;
        }
        std::thread([this, socket = std::move(socket)]() mutable {
            catena::exception_with_status rc("", catena::StatusCode::OK);
            if (!shutdown_) {
                try {
                    // Reading from the socket.
                    SocketReader context;
                    context.read(socket, authorizationEnabled_, version_);
                    std::string requestKey = RESTMethodMap().getForwardMap().at(context.method()) + context.endpoint();
                    // Returning empty response with options to the client if required.
                    if (context.method() == Method_OPTIONS) {
                        rc = catena::exception_with_status("", catena::StatusCode::NO_CONTENT);
                        SocketWriter(socket, context.origin()).sendResponse(rc);
                    // Sending an empty 200 OK response for health check.
                    } else if (context.method() == Method_GET && context.endpoint() == "/health") {
                        SocketWriter(socket, context.origin()).sendResponse(rc);
                    // Otherwise routing to request.
                    } else if (router_.canMake(requestKey)) {
                        std::unique_ptr<ICallData> request = router_.makeProduct(requestKey, this, socket, context, dms_);
                        request->proceed();
                    // ERROR
                    } else { 
                        rc = catena::exception_with_status("Request " + requestKey + " does not exist", catena::StatusCode::UNIMPLEMENTED);
                    }
                // ERROR
                // GCOVR_EXCL_START
                } catch (const catena::exception_with_status& e) {
                    rc = catena::exception_with_status(e.what(), e.status); 
                } catch (const std::invalid_argument& e) {
                    rc = catena::exception_with_status(e.what(), catena::StatusCode::INVALID_ARGUMENT);
                } catch (const std::runtime_error& e) {
                    rc = catena::exception_with_status(e.what(), catena::StatusCode::INTERNAL);
                } catch (const std::exception& e) {
                    rc = catena::exception_with_status(e.what(), catena::StatusCode::UNKNOWN);
                } catch (...) {
                    rc = catena::exception_with_status{"Unknown error", catena::StatusCode::UNKNOWN};
                }
                // GCOVR_EXCL_STOP
            } else {
                rc = catena::exception_with_status{"Service unavailable", catena::StatusCode::UNAVAILABLE};
            }
            // Writing to socket if there was an error.
            if (rc.status != catena::StatusCode::OK) {
                // Try ensures that we don't fail to decrement active requests.
                try {
                    SocketWriter writer(socket);
                    writer.sendResponse(rc);
                } catch (...) {}
            }
            // request completed. Decrementing activeRequests.
            {
                std::lock_guard<std::mutex> lock(activeRequestMutex_);
                activeRequests_ -= 1;
                DEBUG_LOG<<"Active requests remaining: "<<activeRequests_;
            }
        }).detach();
    }
    
    // shutdown_ is true. Send shutdown signal and wait for active requests.
    Connect::shutdownSignal_.emit();
    while(activeRequests_ > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Once active requests are done, close the acceptor and io_context.
    io_context_.stop();
    acceptor_.close();
}

void CatenaServiceImpl::Shutdown() {
    shutdown_ = true;
    // Sending dummy connection to run() loop.
    tcp::socket dummySocket(io_context_);
    dummySocket.connect(tcp::endpoint(tcp::v4(), port_));
};

bool CatenaServiceImpl::registerConnection(catena::common::IConnect* cd) {
    bool canAdd = false;
    std::lock_guard<std::mutex> lock(connectionMutex_);
    // Find the index to insert the new connection based on priority.
    auto it = std::find_if(connectionQueue_.begin(), connectionQueue_.end(),
        [cd](const catena::common::IConnect* connection) { return *cd < *connection; });
    // Based on the iterator, determine if we can add the connection.
    if (connectionQueue_.size() >= maxConnections_) {
        if (it != connectionQueue_.begin()) {
            // Forcefully shutting down lowest priority connection.
            connectionQueue_.front()->shutdown();
            canAdd = true;
        }
    } else {
        canAdd = true;
    }
    // Adding the connection if possible.
    if (canAdd) {
        connectionQueue_.insert(it, cd);
    }
    return canAdd;
}

void CatenaServiceImpl::deregisterConnection(catena::common::IConnect* cd) {
    std::lock_guard<std::mutex> lock(connectionMutex_);
    auto it = std::find_if(connectionQueue_.begin(), connectionQueue_.end(),
                           [cd](const catena::common::IConnect* i) { return i == cd; });
    if (it != connectionQueue_.end()) {
        connectionQueue_.erase(it);
    }
    DEBUG_LOG << "Connected users remaining: " << connectionQueue_.size() << '\n';
}
//Registers current CallData object into the registry
void CatenaServiceImpl::registerItem(ICallData *cd) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    this->registry_.push_back(RegistryItem(cd));
}

//Deregisters current CallData object from the registry
void CatenaServiceImpl::deregisterItem(ICallData *cd) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = std::find_if(registry_.begin(), registry_.end(),
                            [cd](const RegistryItem &i) { return i.get() == cd; });
    if (it != registry_.end()) {
        registry_.erase(it);
    }
    DEBUG_LOG << "Active RPCs remaining: " << registry_.size() << '\n';
}

// (UNUSED)
// GCOVR_EXCL_START
bool CatenaServiceImpl::is_port_in_use_() const {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port_));
        return false;  // Port is not in use
    } catch (const boost::system::system_error& e) {
        return true;  // Port is in use
    }
}
// GCOVR_EXCL_STOP
