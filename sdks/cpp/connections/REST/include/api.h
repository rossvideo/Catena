/*
 * Copyright 2024 Ross Video Ltd
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

/**
 * @file api.h
 * @brief Implements REST API
 * @author unknown
 * @copyright Copyright © 2024 Ross Video Ltd
 */

#pragma once

#define CROW_ENABLE_SSL
#include <crow.h>

// common
#include <Device.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <string>

namespace catena {

/**
 * @brief REST API
 */
class API {

  using Device = catena::common::Device;

  public:
    // explicit API(uint16_t port = 443) : version_{"1.0.0"}, port_{port} {}
    explicit API(Device &dm, uint16_t port = 443);
    virtual ~API() = default;
    API(const API&) = delete;
    API& operator=(const API&) = delete;
    API(API&&) = delete;
    API& operator=(API&&) = delete;

    /**
     * @brief Get the API Version
     *
     * @return std::string
     */
    std::string version() const;

    /**
     * @brief run the API
     */
    void run();

    Device &dm_;

  private:
    std::string version_;
    uint16_t port_;
    crow::SimpleApp app_;

    /**
     * @returns The slots that are populated by dm_.
     */
    crow::response getPopulatedSlots();
    /**
     * @brief The getValue REST call.
     * @param req A crow::request which can be converted into JSON.
     * JSON object should contain keys "slot" and "oid".
     * @returns A crow::response containing the value at the end of oid or an
     * error message if something goes wrong.
     */
    crow::response getValue(const crow::request& req);

  private:
  bool is_port_in_use_() const;
};
}  // namespace catena

// flags for the API
// flags.h
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

// Declare flags for the API
ABSL_DECLARE_FLAG(std::string, certs);
ABSL_DECLARE_FLAG(uint16_t, port);
ABSL_DECLARE_FLAG(bool, mutual_authc);
ABSL_DECLARE_FLAG(bool, authz);
ABSL_DECLARE_FLAG(std::string, static_root);
