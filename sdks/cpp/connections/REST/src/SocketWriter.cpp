#include <SocketWriter.h>
using catena::REST::SocketWriter;
using catena::REST::ChunkedWriter;

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

void SocketWriter::writeOptions() {
    // Writes a response to the client detailing their options for PUT methods.
    std::string headers = "HTTP/1.1 204 No Content\r\n" +
                          CORS_ +
                          "Content-Length: 0\r\n\r\n";
    boost::asio::write(socket_, boost::asio::buffer(headers));
    return;
}

void SocketWriter::finish() {
    finishWithStatus(200);
}

void SocketWriter::finishWithStatus(int status_code) {
    status_code_ = status_code;
    // Always write headers, even for empty responses
    std::string headers = "HTTP/1.1 " + std::to_string(status_code_) + " OK\r\n"
                          "Content-Type: application/json\r\n";
    
    if (!response_.empty()) {
        // Format in a list if this is a multi-part response.
        if (multi_) {
            response_ = "{\n\"response\": [\n" + response_ + "]\n}";
        }
        headers += "Content-Length: " + std::to_string(response_.size()) + "\r\n";
    } else {
        headers += "Content-Length: 0\r\n";
    }
    
    headers += CORS_ + "Connection: close\r\n\r\n";
    boost::asio::write(socket_, boost::asio::buffer(headers + response_));
}

void SocketWriter::finish(google::protobuf::Message& msg) {
    // Finishes the writing process by writing a message to the socket.
    write(msg);
    finish();
}

void ChunkedWriter::writeHeaders(catena::exception_with_status& status) {
    // Setting type depending on if we are writing an error or not.
    std::string type;
    if (status.status == catena::StatusCode::OK) {
        type = "text/event-stream";
    } else {
        type = "text/plain";
    }
    // Writing the headers.
    std::string headers = "HTTP/1.1 " + std::to_string(codeMap_.at(status.status)) + " " + status.what() + "\r\n"
                          "Content-Type: " + type + "\r\n"
                          "Transfer-Encoding: chunked\r\n" +
                          CORS_ +
                          "Connection: keep-alive\r\n\r\n";
    boost::asio::write(socket_, boost::asio::buffer(headers));
    hasHeaders_ = true;
}

void ChunkedWriter::write(google::protobuf::Message& msg) {   
    // Converting the value to JSON.
    std::string json_output;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    auto status = MessageToJsonString(msg, &json_output, options);

    // Writing JSON obj if conversion was successful.
    if (status.ok()) {
        // Writing headers if we haven't already.
        if (!hasHeaders_) {
            catena::exception_with_status ok("", catena::StatusCode::OK);
            writeHeaders(ok);
        }
        // Format as SSE event
        std::string event = "data: " + json_output + "\n\n";
        std::string chunk_size = std::format("{:x}", event.size());
        boost::asio::write(socket_, boost::asio::buffer(chunk_size + "\r\n" + event + "\r\n"));
    // Error, thrown so the call knows the end the process.
    } else {
        throw catena::exception_with_status("Failed to convert protobuf to JSON", catena::StatusCode::INVALID_ARGUMENT);
    }
}

void ChunkedWriter::write(catena::exception_with_status& err) {
    // Writing headers if we haven't already.
    if (!hasHeaders_) { writeHeaders(err); }
    // Writing error message as SSE event
    std::string errMsg = err.what();
    if (!errMsg.empty()) {
        std::string event = "data: " + errMsg + "\n\n";
        boost::asio::write(socket_, boost::asio::buffer(std::format("{:x}", event.size()) + "\r\n" + event + "\r\n"));
    }
    finish();
}

void ChunkedWriter::finish() {
    /* 
     * POSTMAN does not like this line as they do not support chunked encoding.
     * CURL yells at you if you don't have it.
     */
    if (userAgent_.find("Postman") == std::string::npos) {
        boost::asio::write(socket_, boost::asio::buffer("0\r\n\r\n"));
    }
}
