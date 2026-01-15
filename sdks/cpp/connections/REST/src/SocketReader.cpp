
#include <SocketReader.h>
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
}

void SocketReader::read(tcp::socket& socket) {
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

    // Reading from the socket.
    boost::asio::streambuf buffer;
    boost::asio::read_until(socket, buffer, "\r\n\r\n");
    std::istream header_stream(&buffer);

    // Getting the first line from the stream (URL), splitting, and parsing.
    std::string header;
    std::getline(header_stream, header);
    std::string url, httpVersion;
    std::string methodStr;
    std::istringstream(header) >> methodStr >> url >> httpVersion;

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
    
    // Looping through headers to retrieve JWS token and json body len.
    std::size_t contentLength = 0;
    while(std::getline(header_stream, header) && header != "\r") {
        // Parse "<name>: <value>" and handle header name case-insensitively
        std::size_t sep = header.find(':');
        if (sep == std::string::npos) {
            continue;
        }
        std::string name = header.substr(0, sep);
        std::string value = header.substr(sep + 1);
        // Trim leading spaces
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
            value.erase(0, 1);
        }
        // Trim trailing CR and spaces
        while (!value.empty() && (value.back() == '\r' || value.back() == ' ' || value.back() == '\t')) {
            value.pop_back();
        }

        // Getting jwsToken.
        if (service_->authorizationEnabled() && jwsToken_.empty() && iequals_header_name(name, "authorization")) {
            // Expect "Bearer <token>" (keep scheme check case-sensitive as before)
            static const std::string kBearerPrefix = "Bearer ";
            if (value.find(kBearerPrefix) == 0) {
                jwsToken_ = value.substr(kBearerPrefix.length());
            }
        }
        // Getting origin
        else if (origin_.empty() && iequals_header_name(name, "origin")) {
            origin_ = value;
        }
        // Getting detail level from header
        else if (detailLevel_ == st2138::Device_DetailLevel_UNSET && iequals_header_name(name, "detail-level")) {
            std::string dl = value;
            auto& dlMap = catena::common::DetailLevel().getReverseMap();
            if (dlMap.contains(dl)) {
                detailLevel_ = dlMap.at(dl);
            }
        }
        // Getting body content-Length
        else if (contentLength == 0 && iequals_header_name(name, "content-length")) {
            contentLength = stoi(value);
        }
        // Checking Content-Type
        else if (iequals_header_name(name, "content-type") && value != "application/json") {
            throw catena::exception_with_status("Invalid Content-Type", catena::StatusCode::INVALID_ARGUMENT);
        }
    }
    // If body exists, we need to handle leftover data and append the rest.
    if (contentLength > 0) {
        jsonBody_ = std::string((std::istreambuf_iterator<char>(header_stream)), std::istreambuf_iterator<char>());
        if (jsonBody_.size() < contentLength) {
            std::size_t remainingLength = contentLength - jsonBody_.size();
            jsonBody_.resize(contentLength);
            boost::asio::read(socket, boost::asio::buffer(&jsonBody_[contentLength - remainingLength], remainingLength));
        } else if (jsonBody_.size() > contentLength) {
            jsonBody_.resize(contentLength);
        }
    }
    // Setting detail level to NONE if not set.
    if (detailLevel_ == st2138::Device_DetailLevel_UNSET) {
        detailLevel_ = st2138::Device_DetailLevel_NONE;
    }
}
