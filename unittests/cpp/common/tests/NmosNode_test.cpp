#include <gtest/gtest.h>
#include "NmosNode.h"
#include <atomic>
#include <thread>
#include <string>
#include <cstring>
#include <chrono>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <avahi-client/lookup.h>

using namespace catena::common;

extern "C" {
// Immediately “resolve” to 127.0.0.1:3210 and call the provided callback.
AvahiServiceResolver*
__wrap_avahi_service_resolver_new(AvahiClient *c,
                                  AvahiIfIndex interface,
                                  AvahiProtocol protocol,
                                  const char *name,
                                  const char *type,
                                  const char *domain,
                                  AvahiProtocol aproto,
                                  AvahiLookupFlags flags,
                                  AvahiServiceResolverCallback cb,
                                  void *userdata) {
    (void)c; (void)aproto; (void)flags;
    cb(/*resolver*/NULL,
       interface,
       protocol,
       AVAHI_RESOLVER_FOUND,
       name,
       type,
       domain,
       "127.0.0.1",          // host_name
       /*address*/NULL,       // keep NULL so your code skips avahi_address_snprint
       3210,                  // port to match the fake HTTP server
       /*txt*/NULL,           // no TXT → parse_txt loop is skipped
       /*lookup flags*/(AvahiLookupResultFlags)0,
       userdata);
    return (AvahiServiceResolver*)0x1; // any non-null
}

void __wrap_avahi_service_resolver_free(AvahiServiceResolver *r) {
    (void)r; // no-op
}
}

struct FakeRegistry {
    std::atomic<int> resource_posts{0};
    std::atomic<int> heartbeats{0};
    std::atomic<bool> stop{false};
    int listen_fd{-1};
    std::thread thr;

    static void write_resp(int fd, int code) {
        const char* msg = (code == 201 ? "Created" : "OK");
        const char* body = "{}";
        char head[256];
        int n = snprintf(head, sizeof(head),
            "HTTP/1.1 %d %s\r\nContent-Type: application/json\r\n"
            "Content-Length: 2\r\nConnection: close\r\n\r\n", code, msg);
        send(fd, head, n, 0); send(fd, body, 2, 0);
    }

    bool start() {
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0) return false;
        int opt = 1; setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons(3210);
        inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
        if (bind(listen_fd, (sockaddr*)&a, sizeof(a)) < 0) return false;
        if (listen(listen_fd, 16) < 0) return false;
        thr = std::thread([this]{
            while (!stop.load()) {
                sockaddr_in cli{}; socklen_t cl = sizeof(cli);
                int fd = accept(listen_fd, (sockaddr*)&cli, &cl);
                if (fd < 0) { if (stop.load()) break; continue; }
                // Read header enough to get the path (simple & blocking)
                std::string buf; buf.resize(4096);
                ssize_t r = recv(fd, buf.data(), buf.size(), 0);
                if (r <= 0) { close(fd); continue; }
                buf.resize(size_t(r));
                auto sp1 = buf.find(' ');
                auto sp2 = buf.find(' ', sp1 + 1);
                std::string method = buf.substr(0, sp1);
                std::string path = buf.substr(sp1 + 1, sp2 - sp1 - 1);

                if (method == "POST" && path == "/x-nmos/registration/v1.3/resource") {
                    resource_posts++; write_resp(fd, 201);
                } else if (method == "POST" &&
                           path.rfind("/x-nmos/registration/v1.3/health/nodes/", 0) == 0) {
                    heartbeats++; write_resp(fd, 200);
                } else {
                    write_resp(fd, 404);
                }
                close(fd);
            }
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return true;
    }
    void stop_and_join() {
        stop.store(true);
        if (listen_fd >= 0) { shutdown(listen_fd, SHUT_RDWR); close(listen_fd); listen_fd = -1; }
        if (thr.joinable()) thr.join();
    }
};

// A test double that inherits from NmosNode
class TestableNmosNode : public NmosNode {
public:
    // inject fake network identity instead of calling get_node_iface()
    void set_network_identity(const std::string& ip,
                              const std::string& iface,
                              const std::string& mac,
                              const std::string& chassis) {
        ipv4_       = ip;
        iface_      = iface;
        mac_        = mac;
        chassis_id_ = chassis;
    }

    // thin wrapper to call protected/private methods if you need them
    using NmosNode::browse_cb;
    using NmosNode::choose_registry_and_build_base;
    using NmosNode::make_node_json;
    using NmosNode::make_device_json;
    using NmosNode::random_uuid;

    // or a helper to register against a fake base
    bool register_against(const std::string& base,
                          std::chrono::milliseconds hb_interval = std::chrono::milliseconds(100),
                          const std::string& bearer = {}) {
        std::string node_id = random_uuid();
        std::string dev_id  = random_uuid();

        std::string node_json = make_node_json(node_id, 8080);
        std::string dev_json  = make_device_json(dev_id, node_id, 8080);

        const std::string resource_url = base + "/resource";
        bool ok1 = http_post_json(resource_url, node_json, bearer);
        bool ok2 = http_post_json(resource_url, dev_json,  bearer);

        std::thread hb([this, base, node_id, bearer, hb_interval] {
            this->heartbeat_thread(base, node_id, bearer, hb_interval);
        });
            
        std::this_thread::sleep_for(hb_interval * 3);
        stop_.store(true);
        if (hb.joinable()) hb.join();
        stop_.store(false); // reset

        return ok1 && ok2;
    }
};

TEST(NmosNodeTest, RegistersAgainstNoRegistry) {
    TestableNmosNode node;
    node.set_network_identity("127.0.0.1", "lo", "aa-bb-cc-dd-ee-ff", "chassis-123");

    const std::string base = "http://127.0.0.1:3210/x-nmos/registration/v1.3";

    // run against fake HTTP registry (httplib or your mock server)
    bool ok = node.register_against(base);
    EXPECT_FALSE(ok);
}

TEST(NmosNodeTest, Avahi_BrowseThenResolve_ThenRegisterAndHeartbeat) {
    // Start fake registry
    FakeRegistry reg;
    ASSERT_TRUE(reg.start());

    // Prepare node (no Avahi daemon, set identity directly)
    TestableNmosNode node;
    node.set_network_identity("127.0.0.1", "lo", "aa-bb-cc-dd-ee-ff", "chassis-123");

    // Simulate Avahi browse NEW event (this calls avahi_service_resolver_new)
    // Our --wrap immediately fires resolve_cb with host=127.0.0.1, port=3210
    node.browse_cb(/*b*/nullptr,
                   /*iface*/AVAHI_IF_UNSPEC,
                   /*proto*/AVAHI_PROTO_UNSPEC,
                   /*event*/AVAHI_BROWSER_NEW,
                   /*name*/"reg-1",
                   /*type*/"_nmos-registration._tcp",
                   /*domain*/"local",
                   /*flags*/(AvahiLookupResultFlags)0,
                   /*userdata*/&node);

    // Build base URL from discovered candidate
    auto sel = node.choose_registry_and_build_base(8080);
    ASSERT_TRUE(sel.has_value());
    EXPECT_EQ(sel->base, "http://127.0.0.1:3210/x-nmos/registration/v1.3");

    // Register + heartbeat against the fake server
    bool ok = node.register_against(sel->base, std::chrono::milliseconds(120));
    EXPECT_TRUE(ok);
    EXPECT_GE(reg.resource_posts.load(), 2); // node + device
    EXPECT_GE(reg.heartbeats.load(), 1);

    reg.stop_and_join();
}