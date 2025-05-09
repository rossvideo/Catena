#include <SocketWriter.h>
using catena::REST::SocketWriter;
using catena::REST::SSEWriter;

void SocketWriter::sendResponse(const google::protobuf::Message& msg, const catena::exception_with_status& err) {
    // Converting the value to JSON.
    std::string jsonOutput;
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = MessageToJsonString(msg, &jsonOutput, options);
    auto httpStatus = codeMap_.at(err.status);

    // Adding JSON obj to response_ if conversion was successful.
    if (status.ok()) {
        if (response_.empty()) {
            response_ = jsonOutput;
        } else {
            // add comma
            response_ += "," + jsonOutput;
            multi_ = true;
        }
    } else{
        // If conversion failed, set the error status to invalid argument, 
        // which maps to bad request in the codeMap_
        httpStatus = codeMap_.at(catena::StatusCode::INVALID_ARGUMENT);
    }
    
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
            << "Content-Length: " << response_.length() << "\r\n"
            << "Connection: close\r\n"
            << CORS_ 
            << "\r\n";
    
    if (status.ok()) {
        headers << response_ << "\n\n";
    }

    boost::asio::write(socket_, boost::asio::buffer(headers.str()));
}

void SSEWriter::sendResponse(const google::protobuf::Message& msg, const catena::exception_with_status& err) {
    // Converting the value to JSON first
    std::string jsonOutput;
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = MessageToJsonString(msg, &jsonOutput, options);
    auto httpStatus = codeMap_.at(err.status);

    // Adding JSON obj if conversion was successful.
    if (status.ok()) {
        response_ = jsonOutput;
    } else {
        // If conversion failed, set the error status to invalid argument, 
        // which maps to bad request in the codeMap_
        httpStatus = codeMap_.at(catena::StatusCode::INVALID_ARGUMENT);
    }

    // Buffer everything in a single stringstream
    std::stringstream headers;
    headers << "HTTP/1.1 " << httpStatus.first << " " << httpStatus.second << "\r\n"
            << "Content-Type: text/event-stream\r\n"
            << "Cache-Control: no-cache\r\n"
            << "Connection: keep-alive\r\n"
            << "Access-Control-Allow-Origin: " << origin_ << "\r\n"
            << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
            << "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With, Language, Detail-Level\r\n"
            << "Access-Control-Allow-Credentials: true\r\n"
            << "\r\n";
    if (status.ok()) {
        headers << "data: " << response_ << "\n\n";
    }

    boost::asio::write(socket_, boost::asio::buffer(headers.str()));
}
