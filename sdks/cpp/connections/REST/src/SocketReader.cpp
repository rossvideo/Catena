
#include <SocketReader.h>
using catena::REST::SocketReader;

void SocketReader::read(tcp::socket& socket, bool authz) {
    // Resetting variables.
    method_ = "";
    rpc_ = "";
    req_ = "";
    jwsToken_ = "";
    jsonBody_ = "";
    authorizationEnabled_ = authz;

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
        // Getting origin
        else if (header.starts_with("Origin: ")) {
            origin_ = header.substr(std::string("Origin: ").length());
        }
        // Getting user-agent
        else if (header.starts_with("User-Agent: ")) {
            userAgent_ = header.substr(std::string("User-Agent: ").length());
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

void SocketReader::fields(std::unordered_map<std::string, std::string>& fieldMap) const {
    std::string request = req_;
    if (fieldMap.size() == 0) {
        throw catena::exception_with_status("No fields found", catena::StatusCode::INVALID_ARGUMENT);
    } else {
        std::string fieldName = "";
        for (auto& [nextField, value] : fieldMap) {
            // If not the first iteration, find next field and get value of the current one.
            if (fieldName != "") {
                std::size_t end = request.find("/" + nextField + "/");
                if (end == std::string::npos) {
                    throw catena::exception_with_status("Could not find field " + nextField, catena::StatusCode::INVALID_ARGUMENT);
                }
                fieldMap.at(fieldName) = request.substr(0, end);
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
        fieldMap.at(fieldName) = request.substr(0, request.find(" HTTP/1.1"));
    }
}
