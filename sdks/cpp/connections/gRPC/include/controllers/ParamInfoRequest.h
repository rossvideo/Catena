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
 * @brief Implements gRPC ParamInfoRequest controller.
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-02-06
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// connections/gRPC
#include "CallData.h"

// common
#include <ParamVisitor.h>

// type aliases
using catena::common::Path;
using catena::common::ParamVisitor;
using catena::common::IParam;
using catena::common::Authorizer;
using catena::common::IParamVisitor;
using catena::common::timeNow;

namespace catena {
namespace gRPC {

/**
 * @brief CallData class for the ParamInfoRequest gRPC controller.
 */
class ParamInfoRequest : public CallData {
  public:
    /**
     * @brief Constructor for the CallData class of the ParamInfoRequest
     * gRPC. Calls proceed() once initialized.
     *
     * @param service - Pointer to the parent ServiceImpl.
     * @param dms A map of slots to ptrs to their corresponding device.
     * @param ok - Flag to check if the command was successfully executed.
     */ 
    ParamInfoRequest(IServiceImpl *service, SlotMap& dms, bool ok);

    /**
     * @brief Manages the steps of the ParamInfoRequest gRPC command
     * through the state variable status. Returns the value of the
     * parameter specified by the user.
     *
     * @param service - Pointer to the parent ServiceImpl.
     * @param ok - Flag to check if the command was successfully executed.
     */
    void proceed(bool ok) override;

    /**
     * @brief Helper method to add a parameter to the responses
     * @param param The parameter to add
     * @param authz The authorization object
     */
    void addParamToResponses(IParam* param, catena::common::Authorizer& authz);

  private:
    /**
     * @brief Updates the array lengths of the responses.
     * 
     * @param array_name - The name of the array.
     * @param length - The length of the array.
     */
    void updateArrayLengths_(const std::string& array_name, uint32_t length);

    /**
     * @brief The client's scopes.
     */
    std::vector<std::string> clientScopes_;

    /**
     * @brief The request payload.
     */
    catena::ParamInfoRequestPayload req_;

    /**
     * @brief The response payload.
     */
    catena::PushUpdates res_;

    /**
     * @brief gRPC async response writer.
     */
    ServerAsyncWriter<catena::ParamInfoResponse> writer_;
    
    /**
     * @brief The gRPC command's state (kCreate, kProcess, kFinish, etc.).
     */
    CallStatus status_;

    /**
     * @brief A map of slots to ptrs to their corresponding device.
     */
    SlotMap& dms_;
    
    /**
     * @brief The object's unique id.
     */
    int objectId_;

    /**
     * @brief The object's unique id counter.
     */
    static int objectCounter_;
    
    /**
     * @brief The vector of ParamInfoResponse objects.
     */
    std::vector<catena::ParamInfoResponse> responses_;

    /**
     * @brief The current response index.
     */
    uint32_t current_response_{0};

    /**
     * @brief The mutex for the writer lock.
     */
    std::mutex mtx_;

    /**
     * @brief The writer lock.
     */
    std::unique_lock<std::mutex> writer_lock_{mtx_, std::defer_lock};

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
                            std::vector<catena::ParamInfoResponse>& responses,
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
        IDevice& device_;

        /**
         * @brief The authorizer within the visitor
         */
        catena::common::Authorizer& authz_;

        /**
         * @brief The vector of responses within the visitor
         */
        std::vector<catena::ParamInfoResponse>& responses_;

        /**
         * @brief The request payload within the visitor
         */
        ParamInfoRequest& request_;
    };
};

}; // namespace gRPC
}; // namespace catena
