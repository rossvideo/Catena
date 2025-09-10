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

using namespace catena::common;

bool NmosNode::init() {
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
    auto start = std::chrono::steady_clock::now();
    std::thread loop_thr([&](){ avahi_simple_poll_loop(simple_poll_); });
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (stop_.load()) break;
    }
    avahi_simple_poll_quit(simple_poll_);
    loop_thr.join();

    // Choose a registry
    auto sel = choose_registry_and_build_base(8080);
    if (!sel) { LOG(ERROR) << "No registry discovered. Exiting.\n"; return 2; }

    const char* bearerChars = std::getenv("NMOS_BEARER");
    std::string bearer = bearerChars ? std::string(bearerChars) : std::string();

    // Compose minimal resources
    std::string node_id = random_uuid();
    std::string dev_id = random_uuid();
    // TODO: detect local IP to reach your Node API; using 127.0.0.1 as placeholder
    std::string node_json = make_node_json(node_id, 8080);
    std::string dev_json = make_device_json(dev_id, node_id, 8080);
    std::string resource_url = sel->base + "/resource";

    // Register Node then Device (dependency order)
    if (!http_post_json(resource_url, fmt("{\"type\":\"node\",\"data\":%s}", node_json.c_str()), bearer)) {
        LOG(ERROR) << "Failed to register Node\n";
    }
    if (!http_post_json(resource_url, fmt("{\"type\":\"device\",\"data\":%s}", dev_json.c_str()), bearer)) {
        LOG(ERROR) << "Failed to register Device\n";
    }

    // Start heartbeat loop
    std::thread hb(&NmosNode::heartbeat_thread, this, sel->base, node_id, bearer);

    DEBUG_LOG << "Registered Node " << node_id << ". Heartbeating. Press Ctrl+C to exit.";

    // Wait for signal
    while (!stop_.load()) std::this_thread::sleep_for(std::chrono::milliseconds(200));

    if (hb.joinable()) hb.join();

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

std::string NmosNode::now_version_ts() {
    using namespace std::chrono;
    auto tp = system_clock::now();
    auto s = time_point_cast<seconds>(tp);
    auto t = system_clock::to_time_t(s);
    char buf[32]{};
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
    return buf;
}

std::string NmosNode::random_uuid() {
    // NOT cryptographically strong. Good enough for example code.
    auto r32 = [](){ unsigned v = (unsigned)std::rand(); return v; };
    return fmt("%08x-%04x-%04x-%04x-%012x",
    r32(), r32() & 0xFFFF, (r32() & 0x0FFF) | 0x4000,
    (r32() & 0x3FFF) | 0x8000, ((unsigned long long)r32() << 16) | (r32() & 0xFFFF));
}

bool NmosNode::pick_primary_iface(std::string& ifname, std::string& mac){
    DIR* d = opendir("/sys/class/net"); if (!d) return false;
    struct dirent* e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
            std::string name = e->d_name;
        if (name == "lo") continue;
        std::string oper = catena::readFile("/sys/class/net/"+name+"/operstate");
        std::string addr = catena::readFile("/sys/class/net/"+name+"/address");
        if (!oper.empty() && oper == "up") {
            if (!addr.empty()) {
                ifname = name; mac = addr; closedir(d); return true;
            }
        }
    }
    closedir(d);
    return false;
}

bool NmosNode::pick_ipv4_for_iface(const std::string& ifname, std::string& ipv4_out){
    struct ifaddrs *ifaddr=nullptr; if (getifaddrs(&ifaddr)==-1) return false;
    for (auto* ifa=ifaddr; ifa; ifa=ifa->ifa_next){
        if (!ifa->ifa_addr) continue;
        if (!(ifa->ifa_flags & IFF_UP)) continue;
        if (ifname != ifa->ifa_name) continue;
        if (ifa->ifa_addr->sa_family == AF_INET){
            char host[NI_MAXHOST];
            auto* sa = (struct sockaddr_in*)ifa->ifa_addr;
            if (inet_ntop(AF_INET, &sa->sin_addr, host, sizeof(host))){
                ipv4_out = host; freeifaddrs(ifaddr); return true;
            }
        }
    }
    freeifaddrs(ifaddr); return false;
}

// ---------------------- HTTP ----------------------
bool NmosNode::http_post_json(const std::string& url, const std::string& json, const std::string& bearer = {}) {
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
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, json.c_str());
    curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE, (long)json.size());
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

std::string NmosNode::make_node_json(const std::string& node_id, int node_port) {
    std::string ver = now_is04_version();
    std::string ifname = "eth0", mac = "00-00-00-00-00-00";
    std::string ip = "127.0.0.1";            // keep your arg as a hint/fallback
    std::string detected;

    pick_primary_iface(ifname, mac);
    if (pick_ipv4_for_iface(ifname, detected)) ip = detected;

    std::string href = fmt("http://%s:%d/x-nmos/node/v1.3/", ip.c_str(), node_port);

    return fmt(
    "{"
        "\"id\": \"%s\","
        "\"version\": \"%s\","
        "\"label\": \"Catena Node\","
        "\"description\": \"An NMOS node running Catena software\","
        "\"href\": \"%s\","
        "\"caps\": {},"
        "\"tags\": {},"
        "\"services\": ["
            "{"
                "\"href\": \"http://%s:%d/x-catena/status/\","
                "\"type\": \"urn:x-catena:service:status\""
            "}"
        "],"
        "\"api\": {"
            "\"versions\": [ \"v1.3\" ],"
            "\"endpoints\": ["
                "{"
                "\"host\": \"%s\","
                "\"port\": %d,"
                "\"protocol\": \"http\","
                "\"authorization\": false"
                "}"
            "]"
        "},"
        "\"clocks\": ["
        "{"
            "\"name\": \"clk0\","
            "\"ref_type\": \"internal\""
        "}"
        "],"
        "\"interfaces\": ["
        "{"
            "\"name\": \"%s\","
            "\"chassis_id\": \"%s\","
            "\"port_id\": \"%s\""
        "}"
        "]"
    "}",
    node_id.c_str(), ver.c_str(), href.c_str(), ip.c_str(), node_port, ip.c_str(), node_port, ifname.c_str(), mac.c_str(), mac.c_str()
    );
}

// make_device_json: builds a minimal IS-04 Device object
std::string NmosNode::make_device_json(const std::string& dev_id, const std::string& node_id, int node_port) {
    std::string ver = now_is04_version();
    std::string ifname = "eth0", mac = "00-00-00-00-00-00";
    std::string ip = "127.0.0.1";            // keep your arg as a hint/fallback
    std::string detected;

    pick_primary_iface(ifname, mac);
    if (pick_ipv4_for_iface(ifname, detected)) ip = detected;

    return fmt(
    "{"
        "\"id\": \"%s\","
        "\"version\": \"%s\","
        "\"node_id\": \"%s\","
        "\"label\": \"Catena Device\","
        "\"description\": \"Catena Device\","
        "\"type\": \"urn:x-nmos:device:generic\","
        "\"senders\": [],"
        "\"receivers\": [],"
        "\"controls\": ["
            "{"
                "\"type\": \"urn:x-nmos:control:sr-ctrl/v1.1\","
                "\"href\": \"http://%s:%d/x-catena/connection/v1.1/\""
            "}"
        "],"
        "\"tags\": {"
            "\"vendor\": [\"RossVideo\"],"
            "\"model_name\": [\"Catena Model X\"],"
            "\"device_name\": [\"Catena Device 1\"]"
        "}"
    "}",
    dev_id.c_str(), ver.c_str(), node_id.c_str(), ip.c_str(), node_port
    );
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

// ---------------------- main ----------------------
void NmosNode::on_signal(int){ stop_ = true; if (simple_poll_) avahi_simple_poll_quit(simple_poll_); }