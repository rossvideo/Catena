
#include <SocketReader.h>
#include "ServiceImpl.h"
using catena::REST::SocketReader;

namespace catena {
namespace REST {

SocketReader::SocketReader(catena::common::ISubscriptionManager& subscriptionManager, const std::string& EOPath) 
    : subscriptionManager_(subscriptionManager), EOPath_(EOPath) {}

void SocketReader::read(tcp::socket& socket, bool authz) {
    // Resetting variables.
    method_ = "";
    endpoint_ = "";
    fqoid_ = "";
    jwsToken_ = "";
    origin_ = "";
    jsonBody_ = "";
    detailLevel_ = Device_DetailLevel_UNSET;
    authorizationEnabled_ = authz;

    // Reading the headers.
    boost::asio::streambuf buffer;
    boost::asio::read_until(socket, buffer, "\r\n\r\n");
    std::istream header_stream(&buffer);

    // Getting the first line from the stream (URL), splitting, and parsing.
    std::string header;
    std::getline(header_stream, header);
    std::string url, httpVersion;
    std::istringstream(header) >> method_ >> url >> httpVersion;
    url_view u(url);

    // Extracting endpoint_ and slot_ from the url (ex: v1/GetValue/{slot}).
    // Slot is not needed for GetPopulatedSlots and Connect.
    std::string path = u.path();
    try {
        std::vector<std::string> parts;
        catena::split(parts, path, "/");
        slot_ = std::stoi(parts.at(2));

        if (parts.size() > 3) {
            endpoint_ = "/" + parts.at(3);
        }

        //parse fqoid
        if (parts.back() == "stream") {
            parts.pop_back();
        }
        
        for (int i = 4; i < parts.size(); i++) {
            fqoid_ += "/" + parts.at(i);
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
        // authz=false once found to avoid further str comparisons.
        if (authz && jwsToken_.empty() && header.starts_with("Authorization: Bearer ")) {
            jwsToken_ = header.substr(std::string("Authorization: Bearer ").length());
            // Removing newline.
            jwsToken_.erase(jwsToken_.length() - 1);
            authz = false;
        }
        // Getting origin
        else if (origin_.empty() && header.starts_with("Origin: ")) {
            origin_ = header.substr(std::string("Origin: ").length());
        }
        // Getting language
        else if (language_.empty() && header.starts_with("Language: ")) {
            language_ = header.substr(std::string("Language: ").length());
            language_.erase(language_.length() - 1); // Removing newline.
        }
        // Getting detail level from header
        else if (detailLevel_ == Device_DetailLevel_UNSET && header.starts_with("Detail-Level: ")) {
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
    if (detailLevel_ == Device_DetailLevel_UNSET) {
        detailLevel_ = catena::Device_DetailLevel_NONE;
    }
}

}; // Namespace REST
}; // Namespace catena
