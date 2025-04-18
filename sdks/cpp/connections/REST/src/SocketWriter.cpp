
#include <SocketWriter.h>
using catena::REST::SocketWriter;
using catena::REST::SSEWriter;

void SocketWriter::write(google::protobuf::Message& msg) {
    // Adds a JSON message to the end of the response.
    // Converting the value to JSON.
    std::string jsonOutput;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    auto status = MessageToJsonString(msg, &jsonOutput, options);
    // Adding JSON obj to response_ if conversion was successful.
    if (status.ok()) {
        if (response_.empty()) {
            response_ = jsonOutput;
        } else {
            // Delete newline and add comma
            response_.erase(response_.length() - 1);
            response_ += ",\n" + jsonOutput;
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
    // Clearing response_ and sending error.
    response_ = "";
    std::string errMsg = err.what();
    std::string headers = "HTTP/1.1 " + std::to_string(codeMap_.at(err.status)) + " " + err.what() + "\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: " + std::to_string(errMsg.size()) + "\r\n" +
                          CORS_ +
                          "Connection: close\r\n\r\n";
    boost::asio::write(socket_, boost::asio::buffer(headers + errMsg));
}

void SocketWriter::finish() {
    finishWithStatus(200);
}

void SocketWriter::finishWithStatus(int status_code) {
    // Finishes the writing process by writing response to socket.
    if (!response_.empty()) {
        // Format in a list if this is a multi-part response.
        if (multi_) {
            response_ = "{\n\"response\": [\n" + response_ + "]\n}";
        }
        // Set headers and write the response.
        std::string headers = "HTTP/1.1 " + std::to_string(status_code) + "\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: " + std::to_string(response_.size()) + "\r\n" +
                            CORS_ +
                            "Connection: close\r\n\r\n";
        boost::asio::write(socket_, boost::asio::buffer(headers + response_));
    } else {
        // Write just the status code and headers for empty responses
        std::string headers = "HTTP/1.1 " + std::to_string(status_code) + "\r\n"
                            "Content-Length: 0\r\n" +
                            CORS_ +
                            "Connection: close\r\n\r\n";
        boost::asio::write(socket_, boost::asio::buffer(headers));
    }
}

void SocketWriter::finish(google::protobuf::Message& msg) {
    // Finishes the writing process by writing a message to the socket.
    write(msg);
    finish();
}



SSEWriter::SSEWriter(tcp::socket& socket, const std::string& origin, int status_code)
    : socket_{socket} {
    CORS_ = "Access-Control-Allow-Origin: " + origin + "\r\n"
            "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With\r\n"
            "Access-Control-Allow-Credentials: true\r\n";
    
    // Write initial headers with the status code
    std::string headers = "HTTP/1.1 " + std::to_string(status_code) + "\r\n"
                         "Content-Type: text/event-stream\r\n" +
                         CORS_ +
                         "Connection: keep-alive\r\n\r\n";
    boost::asio::write(socket_, boost::asio::buffer(headers));
    hasHeaders_ = true;
}

void SSEWriter::write(google::protobuf::Message& msg) {   
    // Converting the value to JSON.
    std::string json_output;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    auto status = MessageToJsonString(msg, &json_output, options);

    // Writing JSON obj if conversion was successful.
    if (status.ok()) {
        catena::subs(json_output, "\n", "");
        boost::asio::write(socket_, boost::asio::buffer("data: " + json_output + "\n\n"));
    // Error, writing error to the client.
    } else {
        catena::exception_with_status err("Failed to convert protobuf to JSON", catena::StatusCode::INVALID_ARGUMENT);
        write(err);
    }
}

void SSEWriter::write(catena::exception_with_status& err) {
    // If headers haven't been written yet, write them with the error status code
    if (!hasHeaders_) {
        std::string headers = "HTTP/1.1 " + std::to_string(codeMap_.at(err.status)) + "\r\n"
                             "Content-Type: text/event-stream\r\n" +
                             CORS_ +
                             "Connection: keep-alive\r\n\r\n";
        boost::asio::write(socket_, boost::asio::buffer(headers));
        hasHeaders_ = true;
    }
    
    // Writing error message.
    std::string errMsg = err.what();
    boost::asio::write(socket_, boost::asio::buffer("data: " + std::to_string(codeMap_.at(err.status)) + " " + errMsg + "\n\n"));
}

void SSEWriter::finishWithStatus(int status_code) {
    // If headers haven't been written yet, write them with the provided status code
    if (!hasHeaders_) {
        std::string headers = "HTTP/1.1 " + std::to_string(status_code) + "\r\n"
                             "Content-Type: text/event-stream\r\n" +
                             CORS_ +
                             "Connection: keep-alive\r\n\r\n";
        boost::asio::write(socket_, boost::asio::buffer(headers));
        hasHeaders_ = true;
    }
}
