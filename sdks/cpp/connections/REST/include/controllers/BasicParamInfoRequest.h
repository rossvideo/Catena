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
 * @file BasicParamInfoRequest.h
 * @brief Implements REST BasicParamInfoRequest controller.
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-04-22
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>
 
// Connections/REST
#include "ServiceImpl.h"
#include "interface/ISocketReader.h"
#include "SocketWriter.h"
#include "interface/ICallData.h"
#include <IDevice.h>

// common
#include <ParamVisitor.h>
#include <rpc/TimeNow.h>

// Forward declarations
using catena::common::IParam;
using catena::common::IParamVisitor;
using catena::common::timeNow;

namespace catena {
namespace REST {

/**
 * @brief ICallData class for the BasicParamInfoRequest REST controller.
 */
class BasicParamInfoRequest : public ICallData {
  public:
    // Specifying which Device and IParam to use (defaults to catena::...)
    using IDevice = catena::common::IDevice;
    using IParam = catena::common::IParam;

    /**
     * @brief Constructor for the BasicParamInfoRequest controller.
     *
     * @param socket The socket to write the response to.
     * @param context The ISocketReader object.
     * @param dm The device to get the parameter info from.
     */ 
    BasicParamInfoRequest(tcp::socket& socket, ISocketReader& context, IDevice& dm);
    
    /**
     * @brief BasicParamInfoRequest's main process.
     */
    void proceed() override;
    
    /**
     * @brief Finishes the BasicParamInfoRequest process
     */
    void finish() override;
    
    /**
     * @brief Creates a new controller object for use with GenericFactory.
     * 
     * @param socket The socket to write the response stream to.
     * @param context The ISocketReader object.
     * @param dm The device to connect to.
     */
    static ICallData* makeOne(tcp::socket& socket, ISocketReader& context, IDevice& dm) {
      return new BasicParamInfoRequest(socket, context, dm);
    }
    
    
  private:
    /**
     * @brief Writes the current state of the request to the console.
     * @param status The current state of the request (kCreate, kFinish, etc.)
     * @param ok The status of the request (open or closed).
     */
    inline void writeConsole_(CallStatus status, bool ok) const override {
      std::cout << "BasicParamInfoRequest::proceed[" << objectId_ << "]: "
                << timeNow() << " status: "<< static_cast<int>(status)
                <<", ok: "<< std::boolalpha << ok << std::endl;
    }
    /**
     * @brief Helper method to add a parameter to the responses
     * @param param The parameter to add
     */
    void addParamToResponses_(IParam* param, catena::common::Authorizer& authz);
    
    /**
     * @brief Updates the array lengths of the responses.
     * 
     * @param array_name - The name of the array.
     * @param length - The length of the array.
     */
    void updateArrayLengths_(const std::string& array_name, uint32_t length);

    /**
     * @brief The socket to write the response to.
     */
    tcp::socket& socket_;
    
    /**
     * @brief The ISocketReader object.
     */
    ISocketReader& context_;
    
    /**
     * @brief The SSEWriter object for writing to socket_.
     */
    SSEWriter writer_;
    
    /**
     * @brief The error status
     */
    catena::exception_with_status rc_;

    /**
     * @brief The device to get parameter info from.
     */
    IDevice& dm_;

    /**
     * @brief The oid prefix to get parameter info for.
     */
    std::string oid_prefix_;
    
    /**
     * @brief Whether to recursively get parameter info.
     */
    bool recursive_;

    /**
     * @brief ID of the BasicParamInfoRequest object
     */
    int objectId_;
    
    /**
     * @brief The total # of BasicParamInfoRequest objects.
     */
    static int objectCounter_;
    
    /**
     * @brief The vector of BasicParamInfoResponse objects.
     */
    std::vector<catena::BasicParamInfoResponse> responses_;
        
    /**
     * @brief Visitor class for collecting parameter info
     */
    class BasicParamInfoVisitor : public catena::common::IParamVisitor {
        public:
            /**
             * @brief Constructor for the BasicParamInfoVisitor class
             * @param device The device to visit
             * @param authz The authorizer
             * @param responses The vector of responses
             * @param request The request
             */
            BasicParamInfoVisitor(catena::common::IDevice& device, catena::common::Authorizer& authz,
                                std::vector<catena::BasicParamInfoResponse>& responses,
                                BasicParamInfoRequest& request)
                : device_(device), authz_(authz), responses_(responses), request_(request) {}

            /**
             * @brief Visit a parameter
             * @param param The parameter to visit
             * @param path The path of the parameter
             */
            void visit(IParam* param, const std::string& path) override;
            
            /**
             * @brief Visit an array
             * @param param The array to visit
             * @param path The path of the array
             * @param length The length of the array
             */
            void visitArray(IParam* param, const std::string& path, uint32_t length) override;

        private:
            /**
             * @brief The device to visit within the visitor
             */
            catena::common::IDevice& device_;

            /**
             * @brief The authorizer within the visitor
             */
            catena::common::Authorizer& authz_;

            /**
             * @brief The vector of responses within the visitor
             */
            std::vector<catena::BasicParamInfoResponse>& responses_;

            /**
             * @brief The request payload within the visitor
             */
            BasicParamInfoRequest& request_;
    };
};

}; // namespace REST
}; // namespace catena 