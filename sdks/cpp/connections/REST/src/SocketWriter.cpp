
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
        finish(catena::exception_with_status("Failed to convert protobuf to JSON", catena::StatusCode::INVALID_ARGUMENT));
    }
}

void SocketWriter::finish(const catena::exception_with_status& err) {
    // Finishes the writing process with an error message.
    auto httpStatus = codeMap_.at(err.status);
    
    //If response is empty but status is ok, set rc to 204
    if (response_.empty() && err.status == catena::StatusCode::OK) {
        httpStatus = codeMap_.at(catena::StatusCode::NO_CONTENT);
    }

    if (!response_.empty() && multi_) {
        response_ = "{\"response\":[" + response_ + "]}";
    }

    std::stringstream headers;
    headers << "HTTP/1.1 " << httpStatus.first << " " << httpStatus.second << "\r\n"
            << "Content-Type: application/json\r\n"
            << CORS_ << "\r\n"
            << "Content-Length: " << response_.length() << "\r\n"
            << "Connection: close\r\n"
            << "\r\n"
            << response_;

    boost::asio::write(socket_, boost::asio::buffer(headers.str()));
}

void SocketWriter::finish(google::protobuf::Message& msg) {
    // Finishes the writing process by writing a message to the socket.
    write(msg);
    finish(catena::exception_with_status("", catena::StatusCode::OK));
}

SSEWriter::SSEWriter(tcp::socket& socket, const std::string& origin, const catena::exception_with_status& err) : socket_{socket} {
    auto httpStatus = codeMap_.at(err.status);
    std::stringstream headers;
    headers << "HTTP/1.1 " << httpStatus.first << " " << httpStatus.second << "\r\n"
            << "Content-Type: text/event-stream\r\n"
            << "Cache-Control: no-cache\r\n"
            << "Connection: keep-alive\r\n"
            << "Access-Control-Allow-Origin: " << origin << "\r\n"
            << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
            << "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With, Language, Detail-Level\r\n"
            << "Access-Control-Allow-Credentials: true\r\n"
            << "\r\n";

    boost::asio::write(socket_, boost::asio::buffer(headers.str()));
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
        finish(catena::exception_with_status("Failed to convert protobuf to JSON", catena::StatusCode::INVALID_ARGUMENT));
    }
}

void SSEWriter::finish(const catena::exception_with_status& err) {
    // Writing error message.
    auto httpStatus = codeMap_.at(err.status);
    std::stringstream headers;
    headers << "data: " << httpStatus.second << " " << err.what() << "\n\n";
    boost::asio::write(socket_, boost::asio::buffer(headers.str()));
}
