
#include <SocketReader.h>
#include <HTTPServer.h>
#include <string_view>
using catena::REST::SocketReader;

namespace {
static inline bool iequals_header_name(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    const unsigned char* pa = reinterpret_cast<const unsigned char*>(a.data());
    const unsigned char* pb = reinterpret_cast<const unsigned char*>(b.data());
    const std::size_t n = a.size();

    for (std::size_t i = 0; i < n; ++i) {
        unsigned char ca = pa[i];
        unsigned char cb = pb[i]; // already lowercase

        if (ca == cb) continue; // fast path

        // Fold only 'A'..'Z' to 'a'..'z'
        if (ca >= 'A' && ca <= 'Z') {
            ca = static_cast<unsigned char>(ca | 0x20); // to lowercase
            if (ca == cb) continue;
        }

        // If we get here, they are not equal
        return false;
    }
    return true;
}

static inline bool valid_content_type(std::string_view s, std::string_view contentType) {
    if (s.size() < contentType.size()) return false;
    const unsigned char* ps = reinterpret_cast<const unsigned char*>(s.data());
    const unsigned char* pcontentType = reinterpret_cast<const unsigned char*>(contentType.data());
    const std::size_t n = contentType.size();

    for (std::size_t i = 0; i < n; ++i) {
        unsigned char cs = ps[i];
        unsigned char ccontentType = pcontentType[i]; // already lowercase

        if (cs == ccontentType) continue; // fast path

        // Fold only 'A'..'Z' to 'a'..'z'
        if (cs >= 'A' && cs <= 'Z') {
            cs = static_cast<unsigned char>(cs | 0x20); // to lowercase
            if (cs == ccontentType) continue;
        }

        // If we get here, they are not equal
        return false;
    }
    // Ensure MIME type is exactly contentType
    if (n != s.size() && ps[n] != ';') {
        return false;
    }
    return true;

}
}

void SocketReader::read(tcp::socket& socket, uint32_t timeout) {
    // Resetting variables.
    method_ = catena::REST::Method_NONE;
    slot_ = 0;
    endpoint_ = "";
    fqoid_ = "";
    stream_ = false;
    origin_ = "";
    detailLevel_ = st2138::Device_DetailLevel_UNSET;
    jwsToken_ = "";
    jsonBody_ = "";
    requestStart_ = DEFAULT_REQUEST_START;
    requestReceived_ = DEFAULT_REQUEST_RECEIVED;

    // Getting request receival time formatted as,
    // <number of milliseconds since start of epoch>
    const auto epoch_time = std::chrono::system_clock::now().time_since_epoch();
    requestReceived_ = std::chrono::duration_cast<std::chrono::milliseconds>(epoch_time).count();

    bool hasContentType = false; // Used for enforcing Content-Type

    // Reading from the socket using HTTPServer async utilities
    catena::common::HTTPRequest httpRequest;
    auto reader = catena::common::HTTPServer::readRequestAsync(socket, httpRequest, timeout);
    auto result = boost::asio::co_spawn(socket.get_executor(), std::move(reader), boost::asio::use_future);

    try {
        result.get();
    } catch(const catena::exception_with_status& e) {
        throw;
    } catch(const std::runtime_error& e) {
        // Timeout or other error from HTTPServer
        std::string msg = e.what();
        if (msg.find("Timed out") != std::string::npos) {
            throw catena::exception_with_status("Timed out", catena::StatusCode::DEADLINE_EXCEEDED);
        }
        throw catena::exception_with_status("Read error", catena::StatusCode::UNKNOWN);
    } catch(...) {
        throw catena::exception_with_status("Read error", catena::StatusCode::UNKNOWN);
    }

    // Get method, URL, and version from HTTP request
    std::string methodStr = httpRequest.method;
    std::string url = httpRequest.path;

    // Converting method to enum.
    auto& methodMap = RESTMethodMap().getReverseMap();
    if (methodMap.contains(methodStr)) {
        method_ = methodMap.at(methodStr);
    }

    // Parsing url
    url_view u(url);
    try {
        std::vector<std::string> path;
        catena::split(path, u.path(), "/");
        // Checking the url starts with "st2138-api/v1/"
        if (path.size() >= 4 && path[1] == "st2138-api" && path[2] == service_->version()) {
            try {
                slot_ = std::stoi(path[3]);
            } catch(...) {
                endpoint_ = "/" + path[3];
            }
            // If the stream flag was added pop it from the path.
            if (path.back() == "stream") {
                path.pop_back();
                stream_ = true;
            }
            // Lastly getting the endpoint (if not already set) and the fqoid.
            if (path.size() > 4) {
                uint32_t i = 4;
                // First segment after "api/v1/slot" is the endpoint.
                if (endpoint_.empty()) {
                    endpoint_ = "/" + path[i];
                    i++;
                }
                // Everything after the endpoint is the fqoid.
                for (i; i < path.size(); i++) {
                    fqoid_ += "/" + path.at(i);
                }
            }
        } else {
            throw catena::exception_with_status("Invalid URL", catena::StatusCode::INVALID_ARGUMENT);
        }
    } catch (...) {
        throw catena::exception_with_status("Invalid URL", catena::StatusCode::INVALID_ARGUMENT);
    }

    // Parsing query parameters.
    for (auto element : u.encoded_params()) {
        fields_[(std::string)element.key] = element.value;
    }
    
    // Process headers from HTTP request (headers are already lowercase keys)
    std::size_t contentLength = 0;
    
    // Getting jwsToken from Authorization header
    if (service_->authorizationEnabled() && jwsToken_.empty()) {
        std::string authHeader = catena::common::HTTPServer::getHeader(httpRequest.headers, "authorization");
        if (!authHeader.empty()) {
            static const std::string kBearerPrefix = "Bearer ";
            if (authHeader.find(kBearerPrefix) == 0) {
                jwsToken_ = authHeader.substr(kBearerPrefix.length());
            }
        }
    }
    
    // Getting origin
    if (origin_.empty()) {
        origin_ = catena::common::HTTPServer::getHeader(httpRequest.headers, "origin");
    }
    
    // Getting detail level from header
    if (detailLevel_ == st2138::Device_DetailLevel_UNSET) {
        std::string dl = catena::common::HTTPServer::getHeader(httpRequest.headers, "detail-level");
        if (!dl.empty()) {
            auto& dlMap = catena::common::DetailLevel().getReverseMap();
            if (dlMap.contains(dl)) {
                detailLevel_ = dlMap.at(dl);
            }
        }
    }
    
    // Getting time request was sent
    if (requestStart_ == DEFAULT_REQUEST_START) {
        std::string requestStartStr = catena::common::HTTPServer::getHeader(httpRequest.headers, "request-start");
        if (!requestStartStr.empty()) {
            catena::readTimestamp(requestStartStr, requestStart_);
        }
    }
    
    // Getting body content-Length (already validated by HTTPServer::readRequestAsync)
    std::string contentLengthStr = catena::common::HTTPServer::getHeader(httpRequest.headers, "content-length", "0");
    try {
        if (!contentLengthStr.empty() && contentLengthStr[0] != '-') {
            contentLength = std::stoull(contentLengthStr);
        }
    } catch(...) {
        throw catena::exception_with_status("Invalid Content-Length", catena::StatusCode::INVALID_ARGUMENT);
    }
    
    // Checking Content-Type
    std::string contentType = catena::common::HTTPServer::getHeader(httpRequest.headers, "content-type");
    if (!contentType.empty()) {
        if (!valid_content_type(contentType, "application/json")) {
            throw catena::exception_with_status("Invalid Content-Type", catena::StatusCode::INVALID_ARGUMENT);
        }
        hasContentType = true;
    }
    
    // Get body from HTTP request (already read by HTTPServer::readRequestAsync)
    jsonBody_ = httpRequest.body;
    
    // Validate Content-Length matches body size
    if (contentLength <= 0 && jsonBody_.size() > 0) {
        throw catena::exception_with_status("Incorrect Content-Length: data lost", catena::StatusCode::DATA_LOSS);
    }
    if (contentLength > 0) {
        // All request bodies must be json according to the spec
        if (!hasContentType) {
            throw catena::exception_with_status("Content-Type missing", catena::StatusCode::INVALID_ARGUMENT);
        }
        if (jsonBody_.size() != contentLength) {
            throw catena::exception_with_status("Incorrect Content-Length: data lost", catena::StatusCode::DATA_LOSS);
        }
    }
    // Setting detail level to NONE if not set.
    if (detailLevel_ == st2138::Device_DetailLevel_UNSET) {
        detailLevel_ = st2138::Device_DetailLevel_NONE;
    }
}
