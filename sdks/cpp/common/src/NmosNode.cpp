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

using json = nlohmann::json;
using namespace catena::common;

bool NmosNode::init(std::chrono::milliseconds heartbeatInterval) {
    curl_global_init(CURL_GLOBAL_ALL);
    DEBUG_LOG << "Starting Catena REST Discovery Example";

    int error;
    simple_poll_ = avahi_simple_poll_new();
    if (!simple_poll_) {
        LOG(ERROR) << "Failed to create simple poll object.";
        return false;
    }

    DEBUG_LOG << "Starting mDNS discovery for _nmos-registration._tcp services...";

    // Create Avahi client
    client_ = avahi_client_new(
        avahi_simple_poll_get(simple_poll_),
        (AvahiClientFlags)0, client_cb, this, &error);

    if (!client_) {
        LOG(ERROR) << "Failed to create client: " << avahi_strerror(error);
        return false;
    }

    DEBUG_LOG << "Avahi client created successfully.";

    // Browse for _http._tcp services, change as needed
    sb_ = avahi_service_browser_new(
        client_, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
        "_nmos-registration._tcp", nullptr, (AvahiLookupFlags)0, browse_cb, this);

    DEBUG_LOG << "Starting service browser for _nmos-registration._tcp...";

    if (!sb_) {
        LOG(ERROR) << "Failed to create service browser: " << avahi_strerror(avahi_client_errno(client_));
        return false;
    }

    DEBUG_LOG << "Service browser created successfully.";

    // Run discovery for a short window (e.g., 2 seconds) then proceed
    runDiscovery();

    // Choose a registry
    auto sel = choose_registry_and_build_base(8080);
    if (!sel) {
        LOG(ERROR) << "No registry discovered. Exiting.\n";
        return false;
    }

    if (!get_node_iface(candidates_[0].addr)) return false;

    // Compose minimal resources
    node_id_ = random_uuid();
    dev_id_ = random_uuid();

    //send jsons
    if (!sendRequests(sel->base + "/resource", 8080)) {
        LOG(ERROR) << "Failed to register resources. Exiting.\n";
        return false;
    }

    // Start heartbeat loop
    std::thread hb(&NmosNode::heartbeat_thread, this, sel->base, node_id_, bearer_token_, heartbeatInterval);

    DEBUG_LOG << "Registered Node " << node_id_ << ". Heartbeating. Press Ctrl+C to exit.";

    // Wait for signal
    while (!stop_.load()) std::this_thread::sleep_for(std::chrono::milliseconds(200));

    if (hb.joinable()) hb.join();
                                                                                                                                                                                                                                                                       
    return true;
}

void NmosNode::runDiscovery() {
    auto start = std::chrono::steady_clock::now();
    std::thread loop_thr([&](){ avahi_simple_poll_loop(simple_poll_); });
    while (std::chrono::steady_clock::now() - start < discoveryDuration) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (stop_.load()) break;
    }
    avahi_simple_poll_quit(simple_poll_);
    loop_thr.join();
}

bool NmosNode::sendRequests(std::string url, int node_port) {
    json node_json = make_node_json(node_port);
    json dev_json = make_device_json(node_port);

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
                size_t start = 0; while (start < val.size()) { auto pos = val.find(',', start); c.api_versions.emplace_back(val.substr(start, pos - start)); if (pos == std::string::npos) break; start = pos + 1; }
            }
        }
        if (k) avahi_free(k); if (v) avahi_free(v);
    }
}

std::string NmosNode::make_node_json(int node_port) {
    json node_json;

    node_json["type"] = "node";
    node_json["data"] = json::object();
    node_json["data"]["id"] = node_id_;
    node_json["data"]["version"] = now_is04_version();
    node_json["data"]["label"] = node_name_;
    node_json["data"]["description"] = device_desc_;
    node_json["data"]["href"] = fmt("http://%s:%d/x-nmos/node/v1.3/", ipv4_.c_str(), node_port);
    node_json["data"]["caps"] = json::object();
    node_json["data"]["tags"] = json::object();
    node_json["data"]["services"] = json::array({
        {
            {"href", fmt("http://%s:%d/x-catena/status/", ipv4_.c_str(), node_port)},
            {"type", "urn:x-catena:service:status"}
        }
    });
    node_json["data"]["api"] = {
        {"versions", json::array({"v1.3"})},
        {"endpoints", json::array({
            {
                {"host", ipv4_},
                {"port", node_port},
                {"protocol", "http"},
                {"authorization", false}
            }
        })}
    };
    node_json["data"]["clocks"] = json::array({
        {
            {"name", "clk0"},
            {"ref_type", "internal"}
        }
    });
    node_json["data"]["interfaces"] = json::array({
        {
            {"name", iface_},
            {"chassis_id", chassis_id_},
            {"port_id", mac_}
        }
    });
    
    return node_json.dump();
}

// make_device_json: builds a minimal IS-04 Device object
std::string NmosNode::make_device_json(int node_port) {
    json dev_json;

    dev_json["type"] = "device";
    dev_json["data"] = json::object();
    dev_json["data"]["id"] = dev_id_;
    dev_json["data"]["version"] = now_is04_version();
    dev_json["data"]["node_id"] = node_id_;
    dev_json["data"]["label"] = device_name_;
    dev_json["data"]["description"] = device_desc_;
    dev_json["data"]["type"] = "urn:x-nmos:device:generic";
    dev_json["data"]["senders"] = json::array();
    dev_json["data"]["receivers"] = json::array();
    dev_json["data"]["controls"] = json::array({
        {
            {"type", "urn:x-nmos:control:sr-ctrl/v1.1"},
            {"href", fmt("http://%s:%d/x-catena/connection/v1.1/", ipv4_.c_str(), node_port)}
        }
    });
    dev_json["data"]["tags"] = {
        {"vendor", json::array({"RossVideo"})},
        {"model_name", json::array({model_name_})},
        {"device_name", json::array({device_name_})}
    };

    return dev_json.dump();
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
        LOG(ERROR) << "Registry candidate: " << name << " @ " << addr_str << ":" << port
        << " https=" << (node->getCandidates().back().https?"yes":"no") << " pri=" << node->getCandidates().back().priority << "\n";
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
            DEBUG_LOG << "Discovered service: " << name << " of type " << type << " in domain " << domain;
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
    DEBUG_LOG << "Avahi client state changed: " << (int)state;
    if (state == AVAHI_CLIENT_FAILURE) {
        LOG(ERROR) << "Client failure: " << avahi_strerror(avahi_client_errno(client));
        avahi_simple_poll_quit(node->getPoll());
    }
}

// ---------------------- Registration + heartbeat ----------------------

std::optional<NmosNode::RegistrySelection> NmosNode::choose_registry_and_build_base(int node_port) {
    std::lock_guard<std::mutex> lk(cand_mtx_);
    if (candidates_.empty()) return std::nullopt;
    // Prefer lowest priority number, then IPv4 first, then first seen
    std::sort(candidates_.begin(), candidates_.end(), [](const NmosNode::RegistryCandidate& a, const NmosNode::RegistryCandidate& b){
        return a.priority < b.priority;
    });
    const NmosNode::RegistryCandidate& c = candidates_.front();
    std::string ver = "v1.3";
    std::string scheme = c.https ? "https" : "http";
    // We use c.host (SRV target). Some registries advertise by mDNS name, which DNS should resolve.
    std::string base = fmt("%s://%s:%u/x-nmos/registration/%s", scheme.c_str(), c.host.c_str(), c.port, ver.c_str());
    LOG(ERROR) << "Chosen registry base: " << base << "\n";
    return NmosNode::RegistrySelection{base, ver};
}

void NmosNode::heartbeat_thread(std::string base, std::string node_id, std::string bearer, std::chrono::milliseconds interval) {
    std::string url = base + "/health/nodes/" + node_id;
    while (!stop_.load()) {
        (void)http_post_json(url, "{}", bearer);
        std::this_thread::sleep_for(interval);
    }
}