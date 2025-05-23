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
 * @file SetValue.h
 * @brief Implements REST SetValue controller.
 * @author benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// Connections/REST
#include "MultiSetValue.h"

namespace catena {
namespace REST {

/**
 * @brief ICallData class for the SetValue REST controller.
 */
class SetValue : public MultiSetValue {
  // Specifying which Device and IParam to use (defaults to catena::...)
  using IDevice = catena::common::IDevice;
  using IParam = catena::common::IParam;

  public:
    /**
     * @brief Constructor for the SetValue controller.
     *
     * @param socket The socket to write the response to.
     * @param context The ISocketReader object.
     * @param dm The device to set the value in.
     */ 
    SetValue(tcp::socket& socket, ISocketReader& context, IDevice& dm);
    /**
     * @brief Creates a new rest object for use with GenericFactory.
     * 
     * @param socket The socket to write the response stream to.
     * @param context The ISocketReader object.
     * @param dm The device to connect to.
     */
    static ICallData* makeOne(tcp::socket& socket, ISocketReader& context, IDevice& dm) {
      return new SetValue(socket, context, dm);
    }
  private:
    /**
     * @brief Converts the jsonPayload_ to MultiSetValuePayload reqs_.
     * @returns True if successful.
     */
    bool toMulti_() override;

    /**
     * @brief The total # of SetValue objects.
     */
    static int objectCounter_;
};

}; // namespace REST
}; // namespace catena
