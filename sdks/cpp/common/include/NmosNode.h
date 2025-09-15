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

#include <curl/curl.h>
#include <netdb.h> 

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>
#include <iostream>
#include <cstring>

#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_format.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <chrono>
#include <signal.h>
#include <functional>
#include <Logger.h>
#include <IAuthorizer.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>

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
class NmosNode {
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

    /**
     * @brief Construct a new NmosNode object
     * 
     * @param serviceImpl Pointer to the REST service implementation
     */
    NmosNode(){};

    /**
     * @brief Destroy the NmosNode object
     */
    ~NmosNode()
    {
      if (simple_poll_) {
        avahi_simple_poll_free(simple_poll_);
        simple_poll_ = nullptr;
      }
      if (client_) {
        avahi_client_free(client_);
        client_ = nullptr;
      }
      if (sb_) {
        avahi_service_browser_free(sb_);
        sb_ = nullptr;
      }
      stop_ = true;
      curl_global_cleanup();
    }

    bool init(std::chrono::milliseconds heartbeatInterval = std::chrono::seconds(1));

    static std::string now_is04_version();

    static std::string now_version_ts();

    static std::string random_uuid();

    static bool pick_primary_iface(std::string& ifname, std::string& mac);

    static bool pick_ipv4_for_iface(const std::string& ifname, std::string& ipv4_out);

    static bool http_post_json(const std::string& url, const std::string& json, const std::string& bearer);

    static void parse_txt_into_candidate(AvahiStringList* txt, RegistryCandidate& c);

    static std::string make_node_json(const std::string& node_id, int node_port);

    static std::string make_device_json(const std::string& dev_id, const std::string& node_id, int node_port);

    static void resolve_cb(
        AvahiServiceResolver *r,
        AvahiIfIndex interface,
        AvahiProtocol protocol,
        AvahiResolverEvent event,
        const char *name,
        const char *type,
        const char *domain,
        const char *host_name,
        const AvahiAddress *address,
        uint16_t port,
        AvahiStringList *txt,
        AvahiLookupResultFlags flags,
        void* userdata);

    static void browse_cb(
        AvahiServiceBrowser *b,
        AvahiIfIndex interface,
        AvahiProtocol protocol,
        AvahiBrowserEvent event,
        const char *name,
        const char *type,
        const char *domain,
        AvahiLookupResultFlags flags,
        void* userdata);

    static void client_cb(AvahiClient *client, AvahiClientState state, void* userdata);
    
    std::optional<RegistrySelection> choose_registry_and_build_base(int node_port);
    
    void heartbeat_thread(std::string base, std::string node_id, std::string bearer, std::chrono::milliseconds interval = std::chrono::seconds(1));

    void on_signal(int);

    AvahiSimplePoll* getPoll() { return simple_poll_; }

    void addCandidate(RegistryCandidate&& c) {
      std::lock_guard<std::mutex> lk(cand_mtx_);
      candidates_.push_back(std::move(c));
    }
    std::vector<RegistryCandidate> getCandidates() {
      std::lock_guard<std::mutex> lk(cand_mtx_);
      return candidates_;
    }

    AvahiClient* getClient() { return client_; }
  protected:
	  AvahiSimplePoll *simple_poll_ = nullptr;
    AvahiClient* client_ = nullptr;
    AvahiServiceBrowser *sb_ = nullptr;
    std::atomic<bool> stop_ = false;
    std::mutex cand_mtx_;
    std::vector<RegistryCandidate> candidates_;
};

} // namespace common
} // namespace catena
