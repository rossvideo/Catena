#include <SocketWriter.h>
using catena::REST::SocketWriter;
using catena::REST::SSEWriter;

void SocketWriter::sendResponse(const catena::exception_with_status& err, const google::protobuf::Message& msg) {
    auto httpStatus = codeMap_.at(err.status);

    // Convert message to JSON
    std::string jsonOutput = "";
    // Check if message is not Empty so we don't send empty body
    if (httpStatus.first < 300 && msg.GetTypeName() != "catena.Empty")  {
        google::protobuf::util::JsonPrintOptions options; // Default options
        auto status = MessageToJsonString(msg, &jsonOutput, options);

        if (!status.ok()) { // GCOVR_EXCL_START
            /* If conversion fails, this error maps to bad request.
             * This should be impossible to get to without failing on function
             * call first*/
            httpStatus = codeMap_.at(catena::StatusCode::INVALID_ARGUMENT);
            // GCOVR_EXCL_STOP
        } else { // Otherwise add the output to response.
            if (jsonBody_.empty()) {
                jsonBody_ = jsonOutput;
            } else {
                jsonBody_ += "," + jsonOutput;
            }
        }
    }
    // If the no buffer or response is empty we need to send the response.
    if (!buffer_ || jsonOutput.empty()) {
        // In case of an error we clear the body and just send an err code.
        if (httpStatus.first >= 300) {
            jsonBody_ = "";
        }
        // If we are buffering a stream response we need to encapsulate the
        // JSON in an array.
        if (buffer_ && !jsonBody_.empty()) {
            jsonBody_ = "{\"data\":[" + jsonBody_ + "]}";
        }
        // Adding headers and writing to client.
        std::stringstream response;
        response << "HTTP/1.1 " << httpStatus.first << " " << httpStatus.second << "\r\n"
                 << "Content-Type: application/json\r\n"
                 << "Connection: close\r\n"
                 << "Content-Length: " << jsonBody_.length() << "\r\n"
                 << "Access-Control-Allow-Origin: " << origin_ << "\r\n"
                 << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                 << "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With, Language, Detail-Level\r\n"
                 << "Access-Control-Allow-Credentials: true\r\n\r\n"
                 << jsonBody_;
        boost::asio::write(socket_, boost::asio::buffer(response.str()));
    }
}

void SSEWriter::sendResponse(const catena::exception_with_status& err, const google::protobuf::Message& msg) {
    auto httpStatus = codeMap_.at(err.status);
    std::stringstream response;

    // Convert message to JSON
    std::string jsonOutput = "";
    if (msg.GetTypeName() != "catena.Empty")  {
        google::protobuf::util::JsonPrintOptions options; // Default options
        auto status = MessageToJsonString(msg, &jsonOutput, options);

        if (!status.ok()) { // GCOVR_EXCL_START
            /* If conversion fails, this error maps to bad request.
             * This should be impossible to get to without failing on function
             * call first*/
            httpStatus = codeMap_.at(catena::StatusCode::INVALID_ARGUMENT);
            jsonOutput = "";
            // GCOVR_EXCL_STOP
        }
    }

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

    // Only send SSE event if we have valid data
    if (httpStatus.first < 300 && !jsonOutput.empty()) {
        response << "data: " << jsonOutput << "\n\n";
    }

    boost::asio::write(socket_, boost::asio::buffer(response.str()));
}
