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
 * @file NmoNode.h
 * @brief Class for NMOS Node functionality
 * @author Christian Twarog christian.twarog@rossvideo.com
 * @date 2025-09-29
 * @copyright Copyright (c) 2025 Ross Video
 */

#include <curl/curl.h>
#include <netdb.h> 
#include "INmosNode.h"
#include <nlohmann/json.hpp>

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
class NmosNode : public INmosNode {
  public:
    /**
     * @brief Construct a new NmosNode object
     * 
     * @param serviceImpl Pointer to the REST service implementation
     */
    NmosNode(const std::string& device_name = "Catena Device", const std::string& node_name = "Catena Node",
          const std::string& device_desc = "A Catena example Node", const std::string& model_name = "Catena Model") : 
      device_name_(device_name),
      node_name_(node_name),
      device_desc_(device_desc),
      model_name_(model_name) {};

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
      stop_.store(true);
      // wait for heartbeat thread to exit
      if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
      }
      curl_global_cleanup();
    }

    NodeCode init(int port = 8080, std::chrono::milliseconds heartbeatInterval = std::chrono::seconds(1)) override;

    AvahiSimplePoll* getPoll() override { return simple_poll_; }

    void addCandidate(RegistryCandidate&& c) override {
      std::lock_guard<std::mutex> lk(cand_mtx_);
      candidates_.push_back(std::move(c));
    }
    std::vector<RegistryCandidate> getCandidates() override {
      std::lock_guard<std::mutex> lk(cand_mtx_);
      return candidates_;
    }

    AvahiClient* getClient() { return client_; }
    
    std::optional<RegistrySelection> choose_registry_and_build_base(std::string) override;
    
    //Flags
    std::atomic<bool> CLIENT_FAILURE = false;
  protected:
    /**
     * @brief Runs the mDNS discovery process for a set duration.
     * This function will block for the duration of the discovery.
     * The duration is set by the discoveryDuration member variable.
     */
    void runDiscovery();

    /**
     * @brief Sends the registration requests to the registry.
     * @param url The base URL of the registry.
     * @param node_port The port number the node is listening on.
     * @return true if all requests were successful, false otherwise.
     */
    bool sendRequests(std::string, int);

    /**
     * @brief Get the current time in IS-04 version format
     * @return A string representing the current time in IS-04 version format
     */
    static std::string now_is04_version();

    /**
     * @brief Generate a random UUID string
     * @return A string representing a random UUID
     */
    static std::string random_uuid();

    bool get_node_iface(const std::string& reg_addr) override;

    /**
     * @brief Perform an HTTP POST with a JSON body
     * @param url The URL to post to
     * @param jsonObj The JSON object to send as the body
     * @param bearer Optional bearer token for authorization
     * @return true if the POST was successful (HTTP 200, 201, 202, or 204), false otherwise
     */
    static bool http_post_json(const std::string& url, const std::string& jsonObj, const std::string& bearer);

    /**
     * @brief Parse an Avahi TXT record into a RegistryCandidate structure
     * @param txt The Avahi TXT record to parse
     * @param c The RegistryCandidate structure to populate
     */
    static void parse_txt_into_candidate(AvahiStringList* txt, RegistryCandidate& c);

    /**
     * @brief Create the JSON representation of the Node resource
     * @param node_port The port number the node is listening on
     * @return A string containing the JSON representation of the Node resource
     */
    std::string make_node_json(int node_port);

    /**
     * @brief Create the JSON representation of the Device resource
     * @param node_port The port number the node is listening on
     * @return A string containing the JSON representation of the Device resource
     */
    std::string make_device_json(int node_port);

    /**
     * @brief Callback for resolving a service
     * @param r The AvahiServiceResolver object
     * @param interface The network interface index
     * @param protocol The network protocol
     * @param event The AvahiResolverEvent type
     * @param name The service name
     * @param type The service type
     * @param domain The service domain
     * @param host_name The resolved host name
     * @param address The resolved address
     * @param port The resolved port
     * @param txt The resolved TXT record
     * @param flags The AvahiLookupResultFlags
     * @param userdata Pointer to user data (should be a NmosNode instance)
     */
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

    /**
     * @brief Callback for browsing services
     * @param b The AvahiServiceBrowser object
     * @param interface The network interface index
     * @param protocol The network protocol
     * @param event The AvahiBrowserEvent type
     * @param name The service name
     * @param type The service type
     * @param domain The service domain
     * @param flags The AvahiLookupResultFlags
     * @param userdata Pointer to user data (should be a NmosNode instance)
     */
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

    /**
     * @brief Callback for Avahi client state changes
     * @param client The AvahiClient object
     * @param state The new AvahiClientState
     * @param userdata Pointer to user data (should be a NmosNode instance)
     */
    static void client_cb(AvahiClient *client, AvahiClientState state, void* userdata);
    
    void run_heartbeat(std::string base, std::string node_id, std::string bearer, std::chrono::milliseconds interval = std::chrono::seconds(5)) override;

	  AvahiSimplePoll *simple_poll_ = nullptr;
    AvahiClient* client_ = nullptr;
    AvahiServiceBrowser *sb_ = nullptr;
    std::atomic<bool> stop_ = false;
    std::mutex cand_mtx_;
    std::vector<RegistryCandidate> candidates_;
    std::thread heartbeat_thread_;
    std::string iface_ = "eth0"; //default interface
    std::string mac_ = "00-00-00-00-00-00"; //default mac
    std::string ipv4_ = "127.0.0.1"; //default ip
    std::string chassis_id_ = "00-00-00-00-00-00"; //default chassis id
    std::string device_name_;
    std::string node_name_;
    std::string device_desc_;
    std::string model_name_;
    std::string node_id_;
    std::string dev_id_;
    std::string bearer_token_;
    std::chrono::milliseconds discoveryDuration = std::chrono::seconds(2);

  };

} // namespace common
} // namespace catena
