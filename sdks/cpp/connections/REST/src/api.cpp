
// connections/REST
#include <api.h>
using catena::API;

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

void API::run() {
    shutdown_ = false;
    // TLS handled by Envoyproxy
    while (!shutdown_) {
        // Waiting for a connection.
        tcp::socket socket(io_context_);
        acceptor_.accept(socket);
        if (shutdown_) { break;}

        // Should probably keep track of threads like we do in the gRPC ServiceImpl.

        // When a connection has been made, detatch to handle asynchronously.
        std::thread([this, socket = std::move(socket)]() mutable {
            try {
                // Reading from the socket.
                SocketReader context(socket, authorizationEnabled_);
                // Setting up authorizer
                std::shared_ptr<catena::common::Authorizer> sharedAuthz;
                catena::common::Authorizer* authz = nullptr;
                if (authorizationEnabled_) {
                    sharedAuthz = std::make_shared<catena::common::Authorizer>(context.jwsToken());
                    authz = sharedAuthz.get();
                } else {
                    authz = &catena::common::Authorizer::kAuthzDisabled;
                }
                // Routing the request based on the name.
                route(socket, context, authz);

            // Catching errors.
            } catch (catena::exception_with_status& err) {
                SocketWriter writer(socket);
                writer.write(err);
            } catch (...) {
                SocketWriter writer(socket);
                catena::exception_with_status err{"Unknown errror", catena::StatusCode::UNKNOWN};
                writer.write(err);
            }
        }).detach();
    }
    std::cout<<"Exiting loop"<<std::endl;
}

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