
// connections/REST
#include <ServiceImpl.h>
using catena::REST::CatenaServiceImpl;

// REST controllers
#include <controllers/Connect.h>
#include <controllers/GetParam.h>
#include <controllers/MultiSetValue.h>
#include <controllers/SetValue.h>
#include <controllers/DeviceRequest.h>
#include <controllers/GetValue.h>
#include <controllers/GetPopulatedSlots.h>
#include <controllers/AddLanguage.h>
#include <controllers/LanguagePackRequest.h>
#include <controllers/ListLanguages.h>
#include <controllers/BasicParamInfoRequest.h>
#include <controllers/UpdateSubscriptions.h>
#include <controllers/ExecuteCommand.h>

using catena::REST::Connect;

// Defining the port flag from SharedFlags.h
ABSL_FLAG(uint16_t, port, 443, "Catena REST service port");

// expand env variables
void expandEnvVariables(std::string &str) {
    static std::regex env{"\\$\\{([^}]+)\\}"};
    std::smatch match;
    while (std::regex_search(str, match, env)) {
        const char *s = getenv(match[1].str().c_str());
        const std::string var(s == nullptr ? "" : s);
        str.replace(match[0].first, match[0].second, var);
    }
}

CatenaServiceImpl::CatenaServiceImpl(IDevice& dm, std::string& EOPath, bool authz, uint16_t port)
    : version_{"1.0.0"},
      dm_{dm},
      EOPath_{EOPath},
      port_{port},
      authorizationEnabled_{authz},
      acceptor_{io_context_, tcp::endpoint(tcp::v4(), port)},
      router_{Router::getInstance()},
      subscriptionManager_{std::make_unique<catena::common::SubscriptionManager>()} {
    if (authorizationEnabled_) {
        std::cout<<"Authorization enabled."<<std::endl;
    }

    // Initializing the routes for router_.

    router_.addProduct("GET/v1/connect",                    Connect::makeOne);
    router_.addProduct("GET/v1/device-request",             DeviceRequest::makeOne);
    router_.addProduct("PUT/v1/execute-command",            ExecuteCommand::makeOne);
    router_.addProduct("GET/v1/get-populated-slots",        GetPopulatedSlots::makeOne);
    router_.addProduct("GET/v1/get-value",                  GetValue::makeOne);
    router_.addProduct("PUT/v1/multi-set-value",            MultiSetValue::makeOne);
    router_.addProduct("PUT/v1/set-value",                  SetValue::makeOne);
    router_.addProduct("GET/v1/get-param",                  GetParam::makeOne);
    router_.addProduct("GET/v1/language-pack-request",      LanguagePackRequest::makeOne);
    router_.addProduct("GET/v1/list-languages",             ListLanguages::makeOne);
    router_.addProduct("PUT/v1/add-language",               AddLanguage::makeOne);
    router_.addProduct("GET/v1/basic-param-info-request",   BasicParamInfoRequest::makeOne);
    router_.addProduct("PUT/v1/update-subscriptions",       UpdateSubscriptions::makeOne);
}

// Initializing the shutdown signal for all open connections.
vdk::signal<void()> Connect::shutdownSignal_;

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
                    SocketReader context(*subscriptionManager_);
                    context.read(socket, authorizationEnabled_);
                    std::string requestKey = context.method() + context.endpoint();
                    // Returning empty response with options to the client if required.
                    if (context.method() == "OPTIONS") {
                        // Set to 204 No Content if in OPTIONS.
                        rc = catena::exception_with_status("", catena::StatusCode::NO_CONTENT);
                        SocketWriter(socket, context.origin()).sendResponse(rc);
                    // Otherwise routing to request.
                    } else if (router_.canMake(requestKey)) {
                        std::unique_ptr<ICallData> request = router_.makeProduct(requestKey, socket, context, dm_);
                        request->proceed();
                        request->finish();
                    // ERROR
                    } else { 
                        rc = catena::exception_with_status("Request " + requestKey + " does not exist", catena::StatusCode::INVALID_ARGUMENT);
                    }
                // ERROR
                } catch (const catena::exception_with_status& e) {
                    rc = std::move(catena::exception_with_status(e.what(), e.status)); 
                } catch (const std::invalid_argument& e) {
                    rc = catena::exception_with_status(e.what(), catena::StatusCode::INVALID_ARGUMENT);
                } catch (const std::runtime_error& e) {
                    rc = catena::exception_with_status(e.what(), catena::StatusCode::INTERNAL);
                } catch (const std::exception& e) {
                    rc = catena::exception_with_status(e.what(), catena::StatusCode::UNKNOWN);
                } catch (...) {
                    rc = catena::exception_with_status{"Unknown error", catena::StatusCode::UNKNOWN};
                }
            } else {
                rc = catena::exception_with_status{"Service shutdown", catena::StatusCode::CANCELLED};
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
                std::cout<<"Active requests remaining: "<<activeRequests_<<std::endl;
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

bool CatenaServiceImpl::is_port_in_use_() const {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port_));
        return false;  // Port is not in use
    } catch (const boost::system::system_error& e) {
        return true;  // Port is in use
    }
}
