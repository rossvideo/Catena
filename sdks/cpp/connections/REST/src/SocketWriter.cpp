
#include <SocketWriter.h>
using catena::REST::SocketWriter;
using catena::REST::SSEWriter;

void SocketWriter::write(google::protobuf::Message& msg) {
    // Adds a JSON message to the end of the response.
    // Converting the value to JSON.
    std::string jsonOutput;
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = MessageToJsonString(msg, &jsonOutput, options);
    
    // Adding JSON obj to response_ if conversion was successful.
    if (status.ok()) {
        if (response_.empty()) {
            response_ = jsonOutput;
        } else {
            // add comma
            response_ += "," + jsonOutput;
            multi_ = true;
        }
    // Error
    } else {
        catena::exception_with_status err("Failed to convert protobuf to JSON", catena::StatusCode::INVALID_ARGUMENT);
        write(err);
    }
}

void SocketWriter::write(catena::exception_with_status& err) {
    // Writes an error message to the socket.
    // Clearing response_ and gettinge errCode.
    int errCode;
    if (response_.empty() && err.status == catena::StatusCode::OK) {
        errCode = 204;
    } else {
        errCode = codeMap_.at(err.status).first;
    }
    std::string errorMessage = err.what();
    
    // Returning the response.
    std::string headers = "HTTP/1.1 " + std::to_string(errCode) + " " + errorMessage + "\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: " + std::to_string(errorMessage.size()) + "\r\n" +
                          CORS_ +
                          "Connection: close\r\n\r\n";
    boost::asio::write(socket_, boost::asio::buffer(headers + errorMessage));
}

void SocketWriter::finish() {
    // Finishes the writing process by writing response to socket.
    if (!response_.empty()) {
        // Format in a list if this is a multi-part response.
        if (multi_) {
            response_ = "{\"response\":[" + response_ + "]}";
        }
        // Set headers and write the response.
        std::string headers = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: " + std::to_string(response_.size()) + "\r\n" +
                              CORS_ +
                              "Connection: close\r\n\r\n";
        boost::asio::write(socket_, boost::asio::buffer(headers + response_));
    }
}

void SocketWriter::finish(google::protobuf::Message& msg) {
    // Finishes the writing process by writing a message to the socket.
    write(msg);
    finish();
}

void SocketWriter::finish(const catena::exception_with_status& err) {
    // Finishes the writing process by writing a status to the socket.
    auto httpStatus = codeMap_.at(err.status);
    std::stringstream ss;
    ss << "HTTP/1.1 " << httpStatus.first << " " << httpStatus.second << "\r\n"
       << "Content-Type: application/json\r\n"
       << CORS_
       << "Content-Length: " << response_.length() << "\r\n"
       << "\r\n"
       << response_;

    boost::asio::write(socket_, boost::asio::buffer(ss.str()));
}

SSEWriter::SSEWriter(tcp::socket& socket, const std::string& origin, const catena::exception_with_status& err) : socket_{socket} {
    auto httpStatus = codeMap_.at(err.status);
    std::stringstream ss;
    ss << "HTTP/1.1 " << httpStatus.first << " " << httpStatus.second << "\r\n"
       << "Content-Type: text/event-stream\r\n"
       << "Cache-Control: no-cache\r\n"
       << "Connection: keep-alive\r\n"
       << "Access-Control-Allow-Origin: " << origin << "\r\n"
       << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
       << "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With, Language, Detail-Level\r\n"
       << "Access-Control-Allow-Credentials: true\r\n"
       << "\r\n";

    boost::asio::write(socket_, boost::asio::buffer(ss.str()));
}

void SSEWriter::write(google::protobuf::Message& msg) {   
    // Converting the value to JSON.
    std::string jsonOutput;
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = MessageToJsonString(msg, &jsonOutput, options);

    // Writing JSON obj if conversion was successful.
    if (status.ok()) {
        boost::asio::write(socket_, boost::asio::buffer("data: " + jsonOutput + "\n\n"));
    // Error, writting error to the client.
    } else {
        catena::exception_with_status err("Failed to convert protobuf to JSON", catena::StatusCode::INVALID_ARGUMENT);
        write(err);
    }
}

void SSEWriter::write(catena::exception_with_status& err) {
    // Writing error message.
    auto httpStatus = codeMap_.at(err.status);
    std::stringstream ss;
    ss << "data: " << httpStatus.second << " " << err.what() << "\n\n";
    boost::asio::write(socket_, boost::asio::buffer(ss.str()));
}
