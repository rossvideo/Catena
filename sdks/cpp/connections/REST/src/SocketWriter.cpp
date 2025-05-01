
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
    std::string response_ = err.what();
    int errCode;
    if (response_.empty() && err.status == catena::StatusCode::OK) {
        errCode = 204;
    } else {
        errCode = codeMap_.at(err.status).code;
    }
    // Returning the response.
    std::string headers = "HTTP/1.1 " + std::to_string(errCode) + " " + err.what() + "\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: " + std::to_string(response_.size()) + "\r\n" +
                          CORS_ +
                          "Connection: close\r\n\r\n";
    boost::asio::write(socket_, boost::asio::buffer(headers + response_));
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


void SocketWriter::finish(const HttpStatus& status) {
    if (multi_) {
        response_ = "{\"response\":[" + response_ + "]}";
    }
    std::string headers = "HTTP/1.1 " + std::to_string(status.code) + " " + status.reason + "\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: " + std::to_string(response_.size()) + "\r\n" +
                            CORS_ +
                            "Connection: close\r\n\r\n";
    boost::asio::write(socket_, boost::asio::buffer(headers + response_));
}

SSEWriter::SSEWriter(tcp::socket& socket, const std::string& origin, const HttpStatus& status)
    : socket_{socket} {
    std::string headers = "HTTP/1.1 " + std::to_string(status.code) + " " + status.reason + "\r\n"
                          "Content-Type: text/event-stream\r\n"
                          "Access-Control-Allow-Origin: " + origin + "\r\n"
                          "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                          "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With\r\n"
                          "Access-Control-Allow-Credentials: true\r\n"
                          "Connection: keep-alive\r\n\r\n";
    boost::asio::write(socket_, boost::asio::buffer(headers));
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
    std::string errMsg = err.what();
    boost::asio::write(socket_, boost::asio::buffer("data: " + codeMap_.at(err.status).reason + " " + errMsg + "\n\n"));
}
