
#include <SocketReader.h>
using catena::REST::SocketReader;

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
                for (; i < path.size(); i++) {
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
        // Getting jwsToken.
        if (service_->authorizationEnabled() && jwsToken_.empty() && header.starts_with("Authorization: Bearer ")) {
            jwsToken_ = header.substr(std::string("Authorization: Bearer ").length());
            // Removing newline.
            jwsToken_.erase(jwsToken_.length() - 1);
        }
        // Getting origin
        else if (origin_.empty() && header.starts_with("Origin: ")) {
            origin_ = header.substr(std::string("Origin: ").length());
        }
        // Getting detail level from header
        else if (detailLevel_ == st2138::Device_DetailLevel_UNSET && header.starts_with("Detail-Level: ")) {
            std::string dl = header.substr(std::string("Detail-Level: ").length());
            dl.erase(dl.length() - 1); // Removing newline
            auto& dlMap = catena::common::DetailLevel().getReverseMap();
            if (dlMap.contains(dl)) {
                detailLevel_ = dlMap.at(dl);
            }
        }
        // Getting body content-Length
        else if (contentLength == 0 && header.starts_with("Content-Length: ")) {
            contentLength = stoi(header.substr(std::string("Content-Length: ").length()));
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
