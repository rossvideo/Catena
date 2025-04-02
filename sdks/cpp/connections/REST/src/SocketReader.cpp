
#include <SocketReader.h>
using catena::REST::SocketReader;

SocketReader::SocketReader(tcp::socket& socket, bool authz)
    : authorizationEnabled_(authz) {
    
    // Reading the headers.
    boost::asio::streambuf buffer;
    boost::asio::read_until(socket, buffer, "\r\n\r\n");
    std::istream header_stream(&buffer);

    // Getting the first line from the stream (URL).
    std::string header;
    std::getline(header_stream, header);
    // For loops used for (probably negligible) performance boost.
    // Parsing URL for method_.
    std::size_t i = 0;
    for (; i < header.length(); i += 1) {
        if (header[i] == ' ') { break; }
        method_ += header[i];
    }
    // Parsing URL for rpc_.
    bool started = false;
    uint16_t slashNum = 0;
    for (; i < header.length(); i += 1) {
        if (started) {
            if (header[i] == '/') {
                slashNum += 1;
            }
            if (slashNum == 3 || header[i] == ' ') {
                i += 1;
                break;
            }
            rpc_ += header[i];
        } else {
            if (header[i] == '/') {
                started = true;
                i -= 1;
            }
        }
    }
    // Parse fields until " http/..."
    for (; i < header.length(); i += 1) {
        if (header[i] == ' ') {
            break;
        }
        req_ += header[i];
    }
    
    // Looping through headers to retrieve JWS token and json body len.
    std::size_t contentLength = 0;
    while(std::getline(header_stream, header) && header != "\r") {
        // authz=false once found to avoid further str comparisons.
        if (authz && header.starts_with("Authorization: Bearer ")) {
            jwsToken_ = header.substr(std::string("Authorization: Bearer ").length());
            // Removing newline.
            jwsToken_.erase(jwsToken_.length() - 1);
            authz = false;
        }
        // Getting body content-Length
        else if (header.starts_with("Content-Length: ")) {
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
