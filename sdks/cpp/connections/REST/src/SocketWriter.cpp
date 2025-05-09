#include <SocketWriter.h>
using catena::REST::SocketWriter;
using catena::REST::SSEWriter;

void SocketWriter::sendResponse(const catena::exception_with_status& err, const google::protobuf::Message& msg) {
    std::stringstream response;
    auto httpStatus = codeMap_.at(err.status);

    // Write headers
    response << "HTTP/1.1 " << httpStatus.first << " " << httpStatus.second << "\r\n"
            << "Content-Type: application/json\r\n"
            << "Connection: close\r\n"
            << "Access-Control-Allow-Origin: " << origin_ << "\r\n"
            << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
            << "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With, Language, Detail-Level\r\n"
            << "Access-Control-Allow-Credentials: true\r\n";

    // Convert message to JSON
    std::string jsonOutput = "";
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = MessageToJsonString(msg, &jsonOutput, options);

    if (!status.ok()) {
        //If conversion fails, this error maps to bad request
        httpStatus = codeMap_.at(catena::StatusCode::INVALID_ARGUMENT);
        jsonOutput = "";
    }

    // Only write response if we have valid data
    if (httpStatus.first < 300 && !jsonOutput.empty()) {
        response << "Content-Length: " << jsonOutput.length() << "\r\n\r\n"
                << jsonOutput;
        boost::asio::write(socket_, boost::asio::buffer(response.str()));
    }
}

void SSEWriter::sendResponse(const catena::exception_with_status& err, const google::protobuf::Message& msg) {
    auto httpStatus = codeMap_.at(err.status);
    std::stringstream response;

    // Send headers only once
    if (!headers_sent_) {
        response << "HTTP/1.1 " << httpStatus.first << " " << httpStatus.second << "\r\n"
                << "Content-Type: text/event-stream\r\n"
                << "Cache-Control: no-cache\r\n"
                << "Connection: keep-alive\r\n"
                << "Access-Control-Allow-Origin: " << origin_ << "\r\n"
                << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                << "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With, Language, Detail-Level\r\n"
                << "Access-Control-Allow-Credentials: true\r\n\r\n";
        headers_sent_ = true;
    }

    //Convert message to JSON
    std::string jsonOutput;
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = MessageToJsonString(msg, &jsonOutput, options);

    // Only send SSE event if we have valid data
    if (httpStatus.first < 300 && status.ok() && !jsonOutput.empty()) {
        response << "data: " << jsonOutput << "\n\n";
    }

    boost::asio::write(socket_, boost::asio::buffer(response.str()));

}
