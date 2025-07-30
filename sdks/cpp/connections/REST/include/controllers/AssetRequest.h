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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
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

/**
 * @file AssetRequest.h
 * @brief Implements REST AssetRequest controller.
 * @author christian.twarog@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>
 
// common
#include <rpc/TimeNow.h>
#include <Status.h>
#include <IParam.h>
#include <IDevice.h>
#include <utils.h>
#include <Authorizer.h>
#include <Enums.h>

// Connections/REST
#include "interface/ISocketReader.h"
#include "SocketWriter.h"
#include "interface/ICallData.h"

// Standard library
#include <filesystem>

#include <Logger.h>

namespace catena {
namespace REST {

/**
 * @brief ICallData class for the GetParam REST controller.
 */
class AssetRequest : public ICallData {
  public:
    // Specifying which Device to use (defaults to catena::...)
    using IDevice = catena::common::IDevice;
    using SlotMap = catena::common::SlotMap;

    /**
     * @brief Constructor for the GetParam controller.
     *
     * @param socket The socket to write the response to.
     * @param context The ISocketReader object.
     * @param dms A map of slots to ptrs to their corresponding device.
     */ 
    AssetRequest(tcp::socket& socket, ISocketReader& context, SlotMap& dms);

    /**
     * converts the PayloadEncoding enum to a string
     * @param encoding The PayloadEncoding enum value to convert.
     * @return The string representation of the PayloadEncoding.
     */
    static std::string payloadEncodingToString(catena::DataPayload::PayloadEncoding encoding);

    /**
     * @brief GetParam's main process.
     */
    void proceed() override;
    
    /**
     * @brief Creates a new controller object for use with GenericFactory.
     *
     * @param socket The socket to write the response stream to.
     * @param context The ISocketReader object.
     * @param dms A map of slots to ptrs to their corresponding device.
     */
    static ICallData* makeOne(tcp::socket& socket, ISocketReader& context, SlotMap& dms) {
      return new AssetRequest(socket, context, dms);
    }

  protected:
    /**
     * @brief Compresses the input data using the specified window bits.
     * @param input The input data to compress.
     * @param windowBits The window bits to use for compression.
     */
    static void compress(std::vector<uint8_t>& input, int windowBits);

    /**
     * @brief Compresses the input data using deflate compression.
     * @param input The input data to compress.
     */
    static void deflate_compress(std::vector<uint8_t>& input);

    /**
     * @brief Compresses the input data using gzip compression.
     * @param input The input data to compress.
     */
    static void gzip_compress(std::vector<uint8_t>& input);

    /**
     * @brief Decompresses the input data using the specified window bits.
     * @param input The input data to decompress.
     * @param windowBits The window bits to use for decompression.
     */
    static void decompress(std::vector<uint8_t>& input, int windowBits);

    /**
     * @brief Decompresses the input data using deflate compression.
     * @param input The input data to decompress.
     */
    static void deflate_decompress(std::vector<uint8_t>& input);
    
    /**
     * @brief Decompresses the input data using gzip compression.
     * @param input The input data to decompress.
     */
    static void gzip_decompress(std::vector<uint8_t>& input);

    /**
     * @brief Gets the last write time of the file.
     * @param path The path to the file.
     * @param out_time The output time.
     * @return True if the last write time is valid, false otherwise.
     */
    static bool get_last_write_time(const std::string& path, std::time_t& out_time);

    /**
     * @brief Extracts the payload from the context and decompresses it if needed.
     */
    void extractPayload(const std::string& filePath);
    
    /**
     * @brief Writes the current state of the request to the console.
     * 
     * @param status The current state of the request (kCreate, kFinish, etc.)
     * @param ok The status of the request (open or closed).
     */
    inline void writeConsole_(CallStatus status, bool ok) const override {
      DEBUG_LOG << "AssetRequest::proceed[" << objectId_ << "]: "
                << catena::common::timeNow() << " status: "
                << static_cast<int>(status) <<", ok: "<< std::boolalpha << ok;
    }

    /**
     * @brief The socket to write the response to.
     */
    tcp::socket& socket_;
    /**
     * @brief The ISocketReader object.
     */
    ISocketReader& context_;
    /**
     * @brief The SocketWriter object for writing to socket_.
     */
    SocketWriter writer_;
    /**
     * @brief A map of slots to ptrs to their corresponding device.
     */
    SlotMap& dms_;

    /**
     * @brief ID of the object
     */
    int objectId_;
    /**
     * @brief The total # of objects.
     */
    static int objectCounter_;
};

}; // namespace REST
}; // namespace catena
