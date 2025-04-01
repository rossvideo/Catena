
// connections/REST
#include <api.h>
using catena::API;

#include <Connect.h>

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

API::API(Device &dm, std::string& EOPath, bool authz, uint16_t port)
    : version_{"1.0.0"},
      dm_{dm},
      EOPath_{EOPath},
      port_{port},
      authorizationEnabled_{authz},
      acceptor_{io_context_, tcp::endpoint(tcp::v4(), port)} {

    if (authorizationEnabled_) {
        std::cout<<"Authorization enabled."<<std::endl;
    }
}

std::string API::version() const {
  return version_;
}

// Initializing the shutdown signal for all open connections.
vdk::signal<void()> API::Connect::shutdownSignal_;

void API::run() {
    // TLS handled by Envoyproxy
    shutdown_ = false;

    while (!shutdown_) {
        // Waiting for a connection.
        tcp::socket socket(io_context_);
        acceptor_.accept(socket);
        // Once a conections is made, handle async.
        std::thread([this, socket = std::move(socket)]() mutable {
            this->route(socket);
        }).detach();
    }
    
    // Shutting down active RPCs.
    Connect::shutdownSignal_.emit(); // Shutdown active Connect RPCs.
    
    // Wait for active RPCs to finish
    while(activeRpcs_ > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    io_context_.stop();
    acceptor_.cancel();
    acceptor_.close();
}

void API::Shutdown() {
    shutdown_ = true;
    // Sending dummy connection to run() loop.
    tcp::socket dummySocket(io_context_);
    dummySocket.connect(tcp::endpoint(tcp::v4(), port_));
};

std::string API::timeNow() {
    std::stringstream ss;
    auto now = std::chrono::system_clock::now();
    auto now_micros = std::chrono::time_point_cast<std::chrono::microseconds>(now);
    auto epoch = now_micros.time_since_epoch();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(epoch);
    auto now_c = std::chrono::system_clock::to_time_t(now);
    ss << std::put_time(std::localtime(&now_c), "%F %T") << '.' << std::setw(6) << std::setfill('0')
       << micros.count() % 1000000;
    return ss.str();
}

/**
 * @todo move to SocketReader once format is decided.
 */
void API::CallData::parseFields(std::string& request, std::unordered_map<std::string, std::string>& fields) const {
    if (fields.size() == 0) {
        throw catena::exception_with_status("No fields found", catena::StatusCode::INVALID_ARGUMENT);
    } else {
        std::string fieldName = "";
        for (auto& [nextField, value] : fields) {
            // If not the first iteration, find next field and get value of the current one.
            if (fieldName != "") {
                std::size_t end = request.find("/" + nextField + "/");
                if (end == std::string::npos) {
                    throw catena::exception_with_status("Could not find field " + nextField, catena::StatusCode::INVALID_ARGUMENT);
                }
                fields.at(fieldName) = request.substr(0, end);
            }
            // Update for the next iteration.
            fieldName = nextField;
            std::size_t start = request.find("/" + fieldName + "/") + fieldName.size() + 2;
            if (start == std::string::npos) {
                throw catena::exception_with_status("Could not find field " + fieldName, catena::StatusCode::INVALID_ARGUMENT);
            }
            request = request.substr(start);
        }
        // We assume the last field is until the end of the request.
        fields.at(fieldName) = request.substr(0, request.find(" HTTP/1.1"));
    }
}

bool API::is_port_in_use_() const {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port_));
        return false;  // Port is not in use
    } catch (const boost::system::system_error& e) {
        return true;  // Port is in use
    }
}
