/*
 * Copyright 2025 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// common
#include <Tags.h>

#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>
#include <utils.h>

#include <api.h>

#include "absl/flags/flag.h"

#include <iostream>
#include <regex>

using catena::API;

// SockerWriter
void API::SocketWriter::write(google::protobuf::Message& msg) {
    // Converting the value to JSON.
    std::string jsonOutput;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    auto status = MessageToJsonString(msg, &jsonOutput, options);

    // Writing headers and JSON obj if conversion was successful.
    if (status.ok()) {
        std::string headers = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: " + std::to_string(jsonOutput.size()) + "\r\n"
                              "Connection: close\r\n\r\n";
        boost::asio::write(socket_, boost::asio::buffer(headers + jsonOutput));
    // Error
    } else {
        catena::exception_with_status err("Failed to convert protobuf to JSON", catena::StatusCode::INVALID_ARGUMENT);
        write(err);
    }
}

void API::SocketWriter::write(catena::exception_with_status& err) {
    std::string errMsg = err.what();
    std::string headers = "HTTP/1.1 " + std::to_string(codeMap_.at(err.status)) + " " + err.what() + "\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: " + std::to_string(errMsg.size()) + "\r\n"
                          "Connection: close\r\n\r\n";
    boost::asio::write(socket_, boost::asio::buffer(headers + errMsg));
}

// Chunked Writer
void API::ChunkedWriter::writeHeaders(catena::exception_with_status& status) {
    // Setting type depending on if we are writing an error or not.
    std::string type;
    if (status.status == catena::StatusCode::OK) {
        type = "application/json";
    } else {
        type = "text/plain";
    }
    // Writing the headers.
    std::string headers = "HTTP/1.1 " + std::to_string(codeMap_.at(status.status)) + " " + status.what() + "\r\n" +
                          "Content-Type: " + type + "\r\n" +
                          "Transfer-Encoding: chunked\r\n" +
                          "Connection: keep-alive\r\n\r\n";
    boost::asio::write(socket_, boost::asio::buffer(headers));
    hasHeaders_ = true;
}

void API::ChunkedWriter::write(google::protobuf::Message& msg) {   
    // Converting the value to JSON.
    std::string json_output;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    auto status = MessageToJsonString(msg, &json_output, options);

    // Writing JSON obj if conversion was successful.
    if (status.ok()) {
        std::string chunk_size = std::format("{:x}", json_output.size());
        boost::asio::write(socket_, boost::asio::buffer(chunk_size + "\r\n" + json_output + "\r\n"));
    // Error, thrown so the call knows the end the process.
    } else {
        throw catena::exception_with_status("Failed to convert protobuf to JSON", catena::StatusCode::INVALID_ARGUMENT);
    }
}

void API::ChunkedWriter::write(catena::exception_with_status& err) {
    std::string errMsg = err.what();
    // Writing headers if we haven;t already.
    if (!hasHeaders_) {
        writeHeaders(err);
    }
    // Writing error message.
    boost::asio::write(socket_, boost::asio::buffer(std::format("{:x}", errMsg.size()) + "\r\n" + errMsg + "\r\n"));
}

void API::ChunkedWriter::finish() {
    boost::asio::write(socket_, boost::asio::buffer("0\r\n\r\n"));
}
