
#include <SocketReader.h>
using catena::REST::SocketReader;

void SocketReader::read(tcp::socket& socket, bool authz) {
    // Resetting variables.
    method_ = "";
    rpc_ = "";
    req_ = "";
    jwsToken_ = "";
    origin_ = "";
    jsonBody_ = "";
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

    // Extracting rpc_ and slot_ from the url (ex: v1/GetValue/{slot}).
    std::string path = u.path();
    std::size_t pos = path.find_last_of('/');
    rpc_ = path.substr(0, pos);
    try {
        slot_ = std::stoi(path.substr(pos + 1));
    } catch (...) {
        throw catena::exception_with_status("Invalid slot", catena::StatusCode::INVALID_ARGUMENT);
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
        }
    }
}
