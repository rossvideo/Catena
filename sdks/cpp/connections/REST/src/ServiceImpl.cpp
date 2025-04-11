
// connections/REST
#include <ServiceImpl.h>
using catena::REST::CatenaServiceImpl;

// RPCs
#include <controllers/Connect.h>
#include <controllers/MultiSetValue.h>
#include <controllers/SetValue.h>
#include <controllers/DeviceRequest.h>
#include <controllers/GetValue.h>
#include <controllers/GetPopulatedSlots.h>
#include <controllers/AddLanguage.h>
using catena::REST::Connect;

#include "absl/flags/flag.h"

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

CatenaServiceImpl::CatenaServiceImpl(Device &dm, std::string& EOPath, bool authz, uint16_t port)
    : version_{"1.0.0"},
      dm_{dm},
      EOPath_{EOPath},
      port_{port},
      authorizationEnabled_{authz},
      acceptor_{io_context_, tcp::endpoint(tcp::v4(), port)},
      router_{Router::getInstance()} {

    if (authorizationEnabled_) {
        std::cout<<"Authorization enabled."<<std::endl;
    }

    // Initializing the routes for router_.
    router_.addProduct("GET/v1/Connect",           Connect::makeOne);
    router_.addProduct("GET/v1/DeviceRequest",     DeviceRequest::makeOne);
    router_.addProduct("GET/v1/GetPopulatedSlots", GetPopulatedSlots::makeOne);
    router_.addProduct("GET/v1/GetValue",          GetValue::makeOne);
    router_.addProduct("PUT/v1/MultiSetValue",     MultiSetValue::makeOne);
    router_.addProduct("PUT/v1/SetValue",          SetValue::makeOne);
    router_.addProduct("PUT/v1/AddLanguage",       AddLanguage::makeOne);
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
        // Once a conections is made, increment activeRPCs and handle async.
        {
        std::lock_guard<std::mutex> lock(activeRpcMutex_);
        activeRpcs_ += 1;
        }
        std::thread([this, socket = std::move(socket)]() mutable {
            catena::exception_with_status rc("", catena::StatusCode::OK);
            if (!shutdown_) {
                try {
                    // Reading from the socket.
                    SocketReader context;
                    context.read(socket, authorizationEnabled_);
                    std::string rpcKey = context.method() + context.rpc();
                    // Returning options to the client if required.
                    if (context.method() == "OPTIONS") {
                        SocketWriter writer(socket);
                        writer.writeOptions();
                    // Otherwise routing to rpc.
                    } else if (router_.canMake(rpcKey)) {
                        std::unique_ptr<ICallData> rpc = router_.makeProduct(rpcKey, socket, context, dm_);
                        rpc->proceed();
                        rpc->finish();
                    // ERROR
                    } else {
                        rc = catena::exception_with_status("RPC " + rpcKey + " does not exist", catena::StatusCode::INVALID_ARGUMENT);
                    }
                // ERROR
                } catch (...) {
                    rc = catena::exception_with_status{"Unknown error", catena::StatusCode::UNKNOWN};
                }
            } else {
                rc = catena::exception_with_status{"Service shutdown", catena::StatusCode::CANCELLED};
            }
            // Writing to socket if there was an error.
            if (rc.status != catena::StatusCode::OK) {
                SocketWriter writer(socket);
                writer.write(rc);
            }
            // rpc completed. Decrementing activeRPCs.
            {
            std::lock_guard<std::mutex> lock(activeRpcMutex_);
            activeRpcs_ -= 1;
            std::cout<<"Active RPCs remaining: "<<activeRpcs_<<std::endl;
            }
        }).detach();
    }
    
    // shutdown_ is true. Send shutdown signal and wait for active RPCs.
    Connect::shutdownSignal_.emit();
    while(activeRpcs_ > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Once active RPCs are done, close the acceptor and io_context.
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
