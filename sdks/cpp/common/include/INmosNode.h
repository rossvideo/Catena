#pragma once

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

/**
 * @file Authorizer.h
 * @brief Class for handling authorization information
 * @author John R. Naylor john.naylor@rossvideo.com
 * @author John Danen
 * @date 2024-09-18
 * @author Ben Whitten benjamin.whitten@rossvideo.com
 * @author Zuhayr Sarker zuhayr.sarker@rossvideo.com
 * @date 2025-02-20
 * @copyright Copyright (c) 2024 Ross Video
 */

// device model
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/address.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/strlst.h>

//common
#include <Device.h>

/**
 * @brief top level namespace for Catena. Functionality at this scope includes
 * the protoc generated classes.
 * Most everything else is in child namespaces such as common, meta, etc.
 */
namespace catena {
/**
 * @brief Functionality that can be shared between the different connection
 * management types
 */
namespace common {

/**
 * @brief Class for handling authorization information.
 * 
 * This class takes a valid JWS token and extracts the "scope" and "exp" claims
 * to later check client access.
 * The actual authentication portion of the JWS token is handled externally and
 * the class just assumes that the token is good when it recieves it.
 */
class INmosNode {
  public:
    struct RegistryCandidate {
      std::string name;
      std::string host; // SRV target hostname
      std::string addr; // printable address (for info)
      uint16_t port{};
      bool https{false};
      int priority{100}; // lower is better (example)
      std::vector<std::string> api_versions; // from TXT api_ver
    };

    struct RegistrySelection {
      std::string base; // e.g. http://host:port/x-nmos/registration/v1.3
      std::string node_ver; // chosen version string, e.g. v1.3
    };

    INmosNode() = default;

    /**
     * @brief Destroy the NmosNode object
     */
    virtual ~INmosNode() = default;

    virtual bool init(std::chrono::milliseconds) = 0;

    virtual AvahiSimplePoll* getPoll() = 0;

    virtual void addCandidate(RegistryCandidate&&) = 0;

    virtual std::vector<RegistryCandidate> getCandidates() = 0;

    virtual AvahiClient* getClient() = 0;

    virtual bool get_node_iface(const std::string&) = 0;

    virtual std::string make_node_json(int) = 0;

    virtual std::string make_device_json(int) = 0;

  protected:
    virtual void runDiscovery() = 0;

    virtual bool sendRequests(std::string, int) = 0;

    virtual std::optional<RegistrySelection> choose_registry_and_build_base(int) = 0;
    
    virtual void heartbeat_thread(std::string base, std::string, std::string, std::chrono::milliseconds) = 0;
};

} // namespace common
} // namespace catena
