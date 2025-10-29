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

#include <NmosNode.h>
#include <sys/ioctl.h>
#include <jwt-cpp/jwt.h>

using namespace catena::common;

NodeCode NmosNode::init(int port, int32_t heartbeatInterval) {
    LOG(INFO) << "Starting Catena REST Discovery Example";

    int error;
    simple_poll_ = avahi_simple_poll_new();
    if (!simple_poll_) {
        LOG(ERROR) << "Failed to create simple poll object.";
        return NodeCode::POLL_FAILED;
    }

    LOG(INFO) << "Starting mDNS discovery for _nmos-registration._tcp services...";

    // Create Avahi client
    client_ = avahi_client_new(
        avahi_simple_poll_get(simple_poll_),
        (AvahiClientFlags)0, client_cb, this, &error);

    if (!client_) {
        LOG(ERROR) << "Failed to create client: " << avahi_strerror(error);
        return NodeCode::CLIENT_FAILED;
    }

    LOG(INFO) << "Avahi client created successfully.";

    // Browse for _nmos-registration services, change as needed
    if (!CLIENT_FAILURE) {
        sb_ = avahi_service_browser_new(
            client_, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
            "_nmos-registration._tcp", nullptr, (AvahiLookupFlags)0, browse_cb, this);
    }
    else return NodeCode::CLIENT_FAILED;

    LOG(INFO) << "Starting service browser for _nmos-registration._tcp...";

    if (!sb_) {
        LOG(ERROR) << "Failed to create service browser: " << avahi_strerror(avahi_client_errno(client_));
        return NodeCode::NO_SERVICE_BROWSER;
    }

    LOG(INFO) << "Service browser created successfully.";

    // Run discovery for a short window (e.g., 2 seconds) then proceed
    runDiscovery();

    // Choose a registry
    auto sel = choose_registry_and_build_base("v1.3");
    if (!sel) {
        LOG(ERROR) << "No registry discovered. Exiting.\n";
        return NodeCode::REGISTRY_NOT_FOUND;
    }

    if (!get_node_iface(candidates_[0].addr)) return NodeCode::NO_IFACE;

    // Compose minimal resources
    node_id_ = random_uuid();
    dev_id_ = random_uuid();

    //send jsons
    if (!sendRequests(sel->base + "/resource", port)) {
        LOG(ERROR) << "Failed to register resources. Exiting.\n";
        return NodeCode::REGISTRATION_FAILED;
    }

    LOG(INFO) << "Registered Node " << node_id_ << ". Heartbeating. Press Ctrl+C to exit.";

    startHeartbeat(sel->base, heartbeatInterval);

    return NodeCode::OK;
}

void NmosNode::runDiscovery() {
    auto end = std::chrono::steady_clock::now() + discoveryDuration_;
    std::thread loop_thr([&](){ avahi_simple_poll_loop(simple_poll_); });
    while (std::chrono::steady_clock::now() < end) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (stop_.load()) break;
    }
    avahi_simple_poll_quit(simple_poll_);
    loop_thr.join();
}

bool NmosNode::sendRequests(std::string url, int node_port) {
    std::string node_json = make_node_json(node_port);
    std::string dev_json = make_device_json(node_port);

    //create bearer tokens
    const char* bearerChars = std::getenv("NMOS_BEARER");
    bearer_token_ = bearerChars ? std::string(bearerChars) : std::string();

    // Register Node then Device (dependency order)
    if (!http_post_json(url, node_json, bearer_token_)) {
        LOG(ERROR) << "Failed to register Node\n";
        return false;
    }
    if (!http_post_json(url, dev_json, bearer_token_)) {
        LOG(ERROR) << "Failed to register Device\n";
        return false;
    }
    return true;
}

// ---------------------- helpers ----------------------
std::string NmosNode::now_is04_version() {
    using namespace std::chrono;
    auto now = std::chrono::system_clock::now();
    auto secs = time_point_cast<seconds>(now);
    auto ns = duration_cast<nanoseconds>(now - secs).count();
    auto t = system_clock::to_time_t(secs); (void)t;
    return fmt("%lld:%09lld",
        (long long)secs.time_since_epoch().count(),
        (long long)ns);
}

std::string NmosNode::random_uuid() {
    // NOT cryptographically strong. Good enough for example code.
    auto r32 = [](){ uint32_t v = (uint32_t)std::rand(); return v; };
    return fmt("%08x-%04x-%04x-%04x-%012llx",
    r32(), r32() & 0xFFFF, (r32() & 0x0FFF) | 0x4000,
    (r32() & 0x3FFF) | 0x8000, ((uint64_t)r32() << 16) | (r32() & 0xFFFF));
}

//figure out how to get subnet and match it to the current ip in the loop
bool NmosNode::get_node_iface(const std::string& reg_addr){
    std::string subnet = reg_addr.substr(0, reg_addr.rfind('.'));

    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) return false;

    struct ifaddrs *ifaddr=nullptr;
    if (getifaddrs(&ifaddr)==-1) return false;
    
    for (auto* ifa=ifaddr; ifa; ifa=ifa->ifa_next){
        if (!ifa->ifa_addr) continue;
        if (!(ifa->ifa_flags & IFF_UP)) continue;
        if (ifa->ifa_addr->sa_family == AF_INET){
            char host[NI_MAXHOST];
            auto* sa = (struct sockaddr_in*)ifa->ifa_addr;

            //get host ip from current interface
            if (inet_ntop(AF_INET, &sa->sin_addr, host, sizeof(host)) &&
                    subnet == std::string(host).substr(0, std::string(host).rfind('.'))) {
                // Found interface on same /24 as reg_addr
                ipv4_ = host;
                iface_ = ifa->ifa_name;

                // Get MAC via ioctl(SIOCGIFHWADDR)
                struct ifreq ifr;
                std::memset(&ifr, 0, sizeof(ifr));
                std::strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ - 1);

                if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
                    auto* hw = reinterpret_cast<unsigned char*>(ifr.ifr_hwaddr.sa_data);
                    mac_ = fmt("%02x-%02x-%02x-%02x-%02x-%02x",
                        hw[0], hw[1], hw[2], hw[3], hw[4], hw[5]);
                } else {
                    mac_.clear(); // or keep previous / handle error as needed
                }

                freeifaddrs(ifaddr);
                close(fd);
                return true;
            }
        }
    }
    freeifaddrs(ifaddr); return false;
}

// ---------------------- HTTP ----------------------
bool NmosNode::http_post_json(const std::string& url, const std::string& jsonObj, const std::string& bearer = {}) {
    CURL* c = curl_easy_init();
    if (!c) return false;
    struct curl_slist* hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
    if (!bearer.empty()) {
        std::string auth = std::string("Authorization: Bearer ") + bearer;
        hdrs = curl_slist_append(hdrs, auth.c_str());
    }
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, jsonObj.c_str());
    curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE, (long)jsonObj.size());
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 5L);
    CURLcode rc = curl_easy_perform(c);
    long code = 0; curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &code);
    if (rc != CURLE_OK) LOG(ERROR) << "HTTP POST failed: " << curl_easy_strerror(rc) << "\n";
    else LOG(ERROR) << "HTTP POST " << url << " -> " << code << "\n";
    curl_slist_free_all(hdrs); curl_easy_cleanup(c);
    return rc == CURLE_OK && (code == 200 || code == 201 || code == 202 || code == 204);
}


// ---------------------- NMOS data ----------------------

void NmosNode::parse_txt_into_candidate(AvahiStringList* txt, NmosNode::RegistryCandidate& c) {
    for (AvahiStringList* l = txt; l; l = avahi_string_list_get_next(l)) {
        char *k = nullptr, *v = nullptr; size_t vlen = 0;
        if (avahi_string_list_get_pair(l, &k, &v, &vlen) == 0 && k) {
            std::string key(k), val = v ? std::string(v, v + vlen) : std::string();
            if (key == "api_proto") c.https = (val == "https");
            else if (key == "pri") { try { c.priority = std::stoi(val); } catch(...){} }
            else if (key == "api_ver") {
                // comma-separated e.g. "v1.0,v1.1,v1.2,v1.3"
                size_t start = 0;
                while (start < val.size()) {
                    auto pos = val.find(',', start);
                    c.api_versions.emplace_back(val.substr(start, pos - start));
                    if (pos == std::string::npos) break; 
                    start = pos + 1;
                }
            }
        }
        if (k) avahi_free(k); if (v) avahi_free(v);
    }
}

std::string NmosNode::make_node_json(int node_port) {
    picojson::object node_json;
    node_json["type"] = picojson::value("node");

    picojson::object data;
    data["id"] = picojson::value(node_id_);
    data["version"] = picojson::value(now_is04_version());
    data["label"] = picojson::value(node_name_);
    data["description"] = picojson::value(device_desc_);
    data["href"] = picojson::value(fmt("http://%s:%d/x-nmos/node/v1.3/", ipv4_.c_str(), node_port));
    data["caps"] = picojson::value(picojson::object());
    data["tags"] = picojson::value(picojson::object());

    picojson::array services;
    picojson::object service;
    service["href"] = picojson::value(fmt("http://%s:%d/x-catena/status/", ipv4_.c_str(), node_port));
    service["type"] = picojson::value("urn:x-catena:service:status");
    services.push_back(picojson::value(service));
    data["services"] = picojson::value(services);

    picojson::object api;
    picojson::array versions;
    versions.push_back(picojson::value("v1.3"));
    api["versions"] = picojson::value(versions);

    picojson::array endpoints;
    picojson::object endpoint;
    endpoint["host"] = picojson::value(ipv4_);
    endpoint["port"] = picojson::value(double(node_port));
    endpoint["protocol"] = picojson::value("http");
    endpoint["authorization"] = picojson::value(false);
    endpoints.push_back(picojson::value(endpoint));
    api["endpoints"] = picojson::value(endpoints);
    data["api"] = picojson::value(api);

    picojson::array clocks;
    picojson::object clock;
    clock["name"] = picojson::value("clk0");
    clock["ref_type"] = picojson::value("internal");
    clocks.push_back(picojson::value(clock));
    data["clocks"] = picojson::value(clocks);

    picojson::array interfaces;
    picojson::object iface;
    iface["name"] = picojson::value(iface_);
    iface["chassis_id"] = picojson::value(chassis_id_);
    iface["port_id"] = picojson::value(mac_);
    interfaces.push_back(picojson::value(iface));
    data["interfaces"] = picojson::value(interfaces);

    node_json["data"] = picojson::value(data);

    return picojson::value(node_json).serialize();
}

std::string NmosNode::make_device_json(int node_port) {
    picojson::object dev_json;
    dev_json["type"] = picojson::value("device");

    picojson::object data;
    data["id"] = picojson::value(dev_id_);
    data["version"] = picojson::value(now_is04_version());
    data["node_id"] = picojson::value(node_id_);
    data["label"] = picojson::value(device_name_);
    data["description"] = picojson::value(device_desc_);
    data["type"] = picojson::value("urn:x-nmos:device:generic");
    data["senders"] = picojson::value(picojson::array());
    data["receivers"] = picojson::value(picojson::array());

    picojson::array controls;
    picojson::object control;
    control["type"] = picojson::value("urn:x-nmos:control:sr-ctrl/v1.1");
    control["href"] = picojson::value(fmt("http://%s:%d/x-catena/connection/v1.1/", ipv4_.c_str(), node_port));
    controls.push_back(picojson::value(control));
    data["controls"] = picojson::value(controls);

    picojson::object tags;
    tags["vendor"] = picojson::value(picojson::array{picojson::value("RossVideo")});
    tags["model_name"] = picojson::value(picojson::array{picojson::value(model_name_)});
    tags["device_name"] = picojson::value(picojson::array{picojson::value(device_name_)});
    data["tags"] = picojson::value(tags);

    dev_json["data"] = picojson::value(data);

    return picojson::value(dev_json).serialize();
}

// Callback for resolving a service
void NmosNode::resolve_cb(
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
    void* userdata)
{
    NmosNode* node = static_cast<NmosNode*>(userdata);
    if (event == AVAHI_RESOLVER_FOUND) {
        char addr_str[AVAHI_ADDRESS_STR_MAX] = {0};
        if (address) avahi_address_snprint(addr_str, sizeof(addr_str), address);
        NmosNode::RegistryCandidate c;
        c.name = name ? name : "";
        c.host = host_name ? host_name : "";
        c.addr = addr_str;
        c.port = port;
        parse_txt_into_candidate(txt, c);
        node->addCandidate(std::move(c));
        LOG(INFO) << "Registry candidate: " << name << " @ " << addr_str << ":" << port
        << " https=" << (node->getCandidates().back().https?"yes":"no") << " pri=" << node->getCandidates().back().priority;
    } else {
        LOG(ERROR) << "Failed to resolve service: " << avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r)));
    }
    avahi_service_resolver_free(r);
}

// Callback for discovered service
void NmosNode::browse_cb(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AvahiLookupResultFlags flags,
    void* userdata)
{
    NmosNode* node = static_cast<NmosNode*>(userdata);
    switch (event) {
        case AVAHI_BROWSER_NEW:
            LOG(INFO) << "Discovered service: " << name << " of type " << type << " in domain " << domain;
            // Found a new service, resolve it
            if (!(avahi_service_resolver_new(
                node->getClient(), interface, protocol, name, type, domain,
                AVAHI_PROTO_UNSPEC, (AvahiLookupFlags)0, resolve_cb, node)))
            {
                LOG(ERROR) << "Failed to resolve service '" << name << "': " << avahi_strerror(avahi_client_errno(node->getClient()));
            }
            break;
        case AVAHI_BROWSER_FAILURE:
            LOG(ERROR) << "browse failure: " << avahi_strerror(avahi_client_errno(node->getClient())) << "\n";
            avahi_simple_poll_quit(node->getPoll());
            break;
        case AVAHI_BROWSER_REMOVE:
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
        case AVAHI_BROWSER_ALL_FOR_NOW:
            break;
    }
}

// Callback for Avahi client
void NmosNode::client_cb(AvahiClient *client, AvahiClientState state, void* userdata)
{
    auto* node = static_cast<NmosNode*>(userdata);
    LOG(INFO) << "Avahi client state changed: " << (int)state;
    if (state == AVAHI_CLIENT_FAILURE) {
        //log and (TBI) close node ONLY IF not registered
        LOG(ERROR) << "Client failure: " << avahi_strerror(avahi_client_errno(client));
        avahi_simple_poll_quit(node->getPoll());
        node->CLIENT_FAILURE.store(true);
    }
}

// ---------------------- Registration + heartbeat ----------------------

std::optional<NmosNode::RegistrySelection> NmosNode::choose_registry_and_build_base(std::string ver) {
    std::lock_guard<std::mutex> lk(cand_mtx_);
    if (candidates_.empty()) return std::nullopt;
    // Prefer lowest priority number, then IPv4 first, then first seen
    std::sort(candidates_.begin(), candidates_.end(), [](const NmosNode::RegistryCandidate& a, const NmosNode::RegistryCandidate& b){
        return a.priority < b.priority;
    });
    //choose first element in candidates with version == ver
    for (const auto& c : candidates_) {
        if (std::find(c.api_versions.begin(), c.api_versions.end(), ver) != c.api_versions.end()) {
            std::string scheme = c.https ? "https" : "http";
            std::string base = fmt("%s://%s:%u/x-nmos/registration/%s", scheme.c_str(), c.host.c_str(), c.port, ver.c_str());
            LOG(INFO) << "Chosen registry base: " << base;
            return NmosNode::RegistrySelection{base, ver};
        }
    }
    return std::nullopt;
}

void NmosNode::startHeartbeat(std::string base, int32_t interval) {
    std::string url = base + "/health/nodes/" + node_id_;
    heartbeat_->getHeartbeatSignal().connect([this, url]() {
        LOG(INFO) << "Heartbeat sent for Node " << this->node_id_;
        http_post_json(url, "{}", this->bearer_token_);
    });
    heartbeat_->start(interval);
}
