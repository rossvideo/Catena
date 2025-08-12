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
 * @file ParamInfoRequest.h
 * @brief Implements controller for the REST param-info endpoint.
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
#include <Authorizer.h>
#include <rpc/TimeNow.h>

#include <Logger.h>

// Forward declarations
using catena::common::IParam;
using catena::common::IParamVisitor;
using catena::common::ParamVisitor;
using catena::common::Authorizer;
using catena::common::timeNow;

namespace catena {
namespace REST {

/**
 * @brief Controller class for the param-info REST endpoint.
 * 
 * This controller supports one method:
 * 
 * - GET: Returns info about the specifeid parameter as well as its sub
 * paramters if the rescursive flag is set to true. Supports both stream and
 * unary responses.
 */
class ParamInfoRequest : public ICallData {
  public:
    // Specifying which Device and IParam to use (defaults to catena::...)
    using IDevice = catena::common::IDevice;
    using IParam = catena::common::IParam;
    using SlotMap = catena::common::SlotMap;

    /**
     * @brief Constructor for the param-info endpoint controller.
     *
     * @param socket The socket to write the response to.
     * @param context The ISocketReader object used to read the client's request.
     * @param dms A map of slots to ptrs to their corresponding device.
     */ 
    ParamInfoRequest(tcp::socket& socket, ISocketReader& context, SlotMap& dms);
    
    /**
     * @brief The param-info endpoint's main process.
     */
    void proceed() override;
    
    /**
     * @brief Creates a new controller object for use with GenericFactory.
     *
     * @param socket The socket to write the response to.
     * @param context The ISocketReader object used to read the client's request.
     * @param dms A map of slots to ptrs to their corresponding device.
     */
    static ICallData* makeOne(tcp::socket& socket, ISocketReader& context, SlotMap& dms) {
      return new ParamInfoRequest(socket, context, dms);
    }
    
    
  private:
    /**
     * @brief Writes the current state of the request to the console.
     * 
     * @param status The current state of the request (kCreate, kFinish, etc.)
     * @param ok The status of the request (open or closed).
     */
    inline void writeConsole_(CallStatus status, bool ok) const override {
      DEBUG_LOG << "ParamInfoRequest::proceed[" << objectId_ << "]: "
                << timeNow() << " status: "<< static_cast<int>(status)
                <<", ok: "<< std::boolalpha << ok;
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
     * @brief The ISocketReader object used to read the client's request.
     * 
     * This is used to get three things from the client:
     * 
     * - A slot specifying the device containing the parameter to query.
     * 
     * - The oid of the parameter to query. An empty oid indicates the
     * retrieval of the device's top level parameters.
     * 
     * - A flag signifying whether to include sub-parameters.
     */
    ISocketReader& context_;
    /**
     * @brief The SocketWriter object for writing to socket_.
     */
    std::unique_ptr<ISocketWriter> writer_ = nullptr;
    /**
     * @brief The error status
     */
    catena::exception_with_status rc_;
    /**
     * @brief A map of slots to ptrs to their corresponding device.
     */
    SlotMap& dms_;
    /**
     * @brief Whether to also go param info of sub-parameters.
     */
    bool recursive_;

    /**
     * @brief The object's unique id.
     */
    int objectId_;
    /**
     * @brief The total # of param-info endpoint controller objects.
     */
    static int objectCounter_;
    
    /**
     * @brief The vector of ParamInfoResponse objects.
     */
    std::vector<st2138::ParamInfoResponse> responses_;
        
    /**
     * @brief Visitor class for collecting parameter info
     */
    class ParamInfoVisitor : public catena::common::IParamVisitor {
        public:
            /**
             * @brief Constructor for the ParamInfoVisitor class
             * @param device The device to visit
             * @param authz The authorizer
             * @param responses The vector of responses
             * @param request The request
             */
            ParamInfoVisitor(catena::common::IDevice& device, catena::common::Authorizer& authz,
                                std::vector<st2138::ParamInfoResponse>& responses,
                                ParamInfoRequest& request)
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
            std::vector<st2138::ParamInfoResponse>& responses_;

            /**
             * @brief The request payload within the visitor
             */
            ParamInfoRequest& request_;
    };
};

}; // namespace REST
}; // namespace catena 