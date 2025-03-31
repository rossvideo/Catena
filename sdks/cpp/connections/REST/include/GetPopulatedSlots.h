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
 * @file GetPopulatedSlots.h
 * @brief Implements REST GetPopulatedSlots RPC.
 * @author benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

 #pragma once

 // protobuf
 #include <interface/device.pb.h>
 #include <google/protobuf/util/json_util.h>
  
 // Connections/REST
 #include "api.h"
 #include "SocketWriter.h"
 using catena::API;
 
 /**
  * @brief CallData class for the GetPopulatedSlots REST RPC.
  */
 class API::GetPopulatedSlots : public CallData {
   public:
     /**
      * @brief Constructor for the GetPopulatedSlots RPC. Calls proceed() once
      * initialized.
      *
      * @param jsonPayload The json body extracted from the request.
      * @param socket The socket to write the response to.
      * @param dm The device to get the value from.
      * @param authz The authorizer object containing the client's scopes.
      */ 
     GetPopulatedSlots(tcp::socket& socket, Device& dm);
   private:
     /**
      * @brief GetPopulatedSlots's main process.
      */
     void proceed() override;
     /**
      * @brief Finishes the GetPopulatedSlots process.
      */
     void finish() override;
 
     /**
      * @brief The socket to write the response to.
      */
     tcp::socket& socket_;
     /**
      * @brief The SocketWriter object for writing to socket_.
      */
     SocketWriter writer_;
     /**
      * @brief The device to get slot of.
      */
     Device& dm_;
 
     /**
      * @brief ID of the GetPopulatedSlots object
      */
     int objectId_;
     /**
      * @brief The total # of GetPopulatedSlots objects.
      */
     static int objectCounter_;
 };
 