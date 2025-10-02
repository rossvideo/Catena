#include <gtest/gtest.h>
#include "NmosNode.h"
#include <atomic>
#include <thread>
#include <cstring>
#include <chrono>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "WrapAvahiClient.h"

inline AvahiTestControl g_avahi_test_control;

using namespace catena::common;

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
                    std::cout << "Heartbeat received: " << heartbeats << "\n";
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
    TestableNmosNode(const std::string& device_name = "Test Catena Device", const std::string& node_name = "Test Catena Node",
          const std::string& device_desc = "A Test Catena example Node", const std::string& model_name = "Test Catena Model") : 
      NmosNode(device_name, node_name, device_desc, model_name)
    {
        discoveryDuration = std::chrono::milliseconds(10);
    };
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
    using NmosNode::resolve_cb;
    using NmosNode::choose_registry_and_build_base;
    using NmosNode::make_node_json;
    using NmosNode::make_device_json;
    using NmosNode::random_uuid;
    using NmosNode::parse_txt_into_candidate;
    using NmosNode::stop_;

    // or a helper to register against a fake base
    bool register_against(const std::string& base,
                          std::chrono::milliseconds hb_interval = std::chrono::milliseconds(100),
                          const std::string& bearer = {}) {
        node_id_ = random_uuid();
        dev_id_  = random_uuid();

        std::string node_json = make_node_json(8080);
        std::string dev_json  = make_device_json(8080);

        const std::string resource_url = base + "/resource";
        bool ok1 = http_post_json(resource_url, node_json, bearer);
        bool ok2 = http_post_json(resource_url, dev_json,  bearer);

        std::thread hb([this, base, bearer, hb_interval] {
            this->run_heartbeat(base, node_id_, bearer, hb_interval);
        });
            
        std::this_thread::sleep_for(hb_interval * 3);
        stop_.store(true);
        if (hb.joinable()) hb.join();
        stop_.store(false); // reset

        return ok1 && ok2;
    }
};
class NmosNodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_avahi_test_control.reset();
    }

    void TearDown() override {
    }

    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("NmosNodeTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
};


TEST_F(NmosNodeTest, SimplePollCreationFail) {
    TestableNmosNode node;
    g_avahi_test_control.simple_poll_new_return = nullptr; // force failure
    EXPECT_EQ(node.init(), NodeCode::POLL_FAILED);
}

TEST_F(NmosNodeTest, ClientCreationInstanceFail) {
    TestableNmosNode node;
    g_avahi_test_control.client_new_return = nullptr; // force failure
    EXPECT_EQ(node.init(), NodeCode::CLIENT_FAILED);
}

TEST_F(NmosNodeTest, ClientCreationCbFail) {
    TestableNmosNode node;
    g_avahi_test_control.client_state = AVAHI_CLIENT_FAILURE; // force failure in callback
    //verify client_cb AVAHI_CLIENT_FAILURE case is executed
    EXPECT_EQ(node.init(), NodeCode::CLIENT_FAILED);
    EXPECT_EQ(g_avahi_test_control.simple_poll_quit, 1);
}

TEST_F(NmosNodeTest, BrowserCreationFail) {
    TestableNmosNode node;
    g_avahi_test_control.service_browser_new_return = nullptr; // normal
    EXPECT_EQ(node.init(), NodeCode::NO_SERVICE_BROWSER);
}

TEST_F(NmosNodeTest, SelectRegistryCandidateFailed) {
    //by default the wrapped avahi functions will not have populated registry candidates list yet
    //so init() should fail due to no candidates found during runDiscovery()
    TestableNmosNode node;
    EXPECT_EQ(node.init(), NodeCode::REGISTRY_NOT_FOUND);
    EXPECT_EQ(node.getCandidates().size(), 0);
}

TEST_F(NmosNodeTest, DiscoveredRegistryButNoIface) {
    TestableNmosNode node;

    g_avahi_test_control.discovered_service = true;
    g_avahi_test_control.has_iface = false; // force get_node_iface() to fail
    
    EXPECT_EQ(node.init(), NodeCode::NO_IFACE);
}

TEST_F(NmosNodeTest, DiscoveredRegistryFailedRequests) {
    TestableNmosNode node;

    g_avahi_test_control.discovered_service = true;
    g_avahi_test_control.has_iface = true;

    EXPECT_EQ(node.init(), NodeCode::REGISTRATION_FAILED);
}

TEST_F(NmosNodeTest, NmosNodeTest_DiscoveredRegistryBrowserFailure) {
    TestableNmosNode node;

    g_avahi_test_control.discovered_service = true;
    g_avahi_test_control.has_iface = true;
    g_avahi_test_control.browse_event = AVAHI_BROWSER_FAILURE; // force browse failure

    EXPECT_EQ(node.init(), NodeCode::REGISTRY_NOT_FOUND);
    EXPECT_EQ(g_avahi_test_control.simple_poll_quit, 2);
}

TEST_F(NmosNodeTest, NmosNodeTest_DiscoveredRegistryBrowserRemove) {
    TestableNmosNode node;

    g_avahi_test_control.discovered_service = true;
    g_avahi_test_control.has_iface = true;
    g_avahi_test_control.browse_event = AVAHI_BROWSER_REMOVE; // force browse failure

    EXPECT_EQ(node.init(), NodeCode::REGISTRY_NOT_FOUND);
    EXPECT_EQ(g_avahi_test_control.simple_poll_quit, 2);
}

TEST_F(NmosNodeTest, NmosNodeTest_DiscoveredRegistryBrowserCacheExhausted) {
    TestableNmosNode node;

    g_avahi_test_control.discovered_service = true;
    g_avahi_test_control.has_iface = true;
    g_avahi_test_control.browse_event = AVAHI_BROWSER_CACHE_EXHAUSTED; // force browse failure

    EXPECT_EQ(node.init(), NodeCode::REGISTRY_NOT_FOUND);
    EXPECT_EQ(g_avahi_test_control.simple_poll_quit, 2);
}

TEST_F(NmosNodeTest, NmosNodeTest_DiscoveredRegistryBrowserAllForNow) {
    TestableNmosNode node;

    g_avahi_test_control.discovered_service = true;
    g_avahi_test_control.has_iface = true;
    g_avahi_test_control.browse_event = AVAHI_BROWSER_ALL_FOR_NOW ; // force browse failure

    EXPECT_EQ(node.init(), NodeCode::REGISTRY_NOT_FOUND);
    EXPECT_EQ(g_avahi_test_control.simple_poll_quit, 2);
}

TEST_F(NmosNodeTest, DiscoveredRegistryNoHeartbeat) {
    TestableNmosNode node;
    FakeRegistry reg;
    node.stop_.store(true); // prevent heartbeat thread from running

    g_avahi_test_control.discovered_service = true;
    g_avahi_test_control.has_iface = true;

    ASSERT_TRUE(reg.start());
    EXPECT_EQ(node.init(8080, std::chrono::milliseconds(20)), NodeCode::OK);
    EXPECT_EQ(reg.resource_posts.load(), 2); // node + device
    EXPECT_EQ(reg.heartbeats.load(), 0); // no heartbeat thread

    reg.stop_and_join();
}

TEST_F(NmosNodeTest, DiscoveredRegistryHeartbeat) {
    TestableNmosNode node;
    FakeRegistry reg;

    g_avahi_test_control.discovered_service = true;
    g_avahi_test_control.has_iface = true;

    //start thread that kills heartbeat after a couple beats
    std::thread killer([&]{
        while (reg.heartbeats.load() < 2) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        node.stop_.store(true);
        reg.stop_and_join();
    });

    ASSERT_TRUE(reg.start());
    EXPECT_EQ(node.init(8080, std::chrono::milliseconds(20)), NodeCode::OK);

    if (killer.joinable()) killer.join();
    EXPECT_GE(reg.heartbeats.load(), 2);
}

TEST_F(NmosNodeTest, AvahiBrowseWrongHost) {
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
                   /*type*/"fake_nonexistant_type",
                   /*domain*/"local",
                   /*flags*/(AvahiLookupResultFlags)0,
                   /*userdata*/&node);

    // Build base URL from discovered candidate
    auto sel = node.choose_registry_and_build_base("v1.3");
    ASSERT_FALSE(sel.has_value());

    reg.stop_and_join();
}

TEST_F(NmosNodeTest, AvahiBrowseThenResolveTrue) {
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
    auto sel = node.choose_registry_and_build_base("v1.3");
    ASSERT_TRUE(sel.has_value());
    EXPECT_EQ(sel->base, "http://127.0.0.1:3210/x-nmos/registration/v1.3");

    // Register + heartbeat against the fake server
    bool ok = node.register_against(sel->base, std::chrono::milliseconds(120));
    EXPECT_TRUE(ok);
    EXPECT_GE(reg.resource_posts.load(), 2); // node + device
    EXPECT_GE(reg.heartbeats.load(), 1);

    reg.stop_and_join();
}

TEST_F(NmosNodeTest, ParseTxtIntoCandidate) {
    // Create a dummy AvahiStringList with one key-value pair for parse_txt_into_candidate
    AvahiStringList* txt = avahi_string_list_new("api_ver=v1.0,v1.1,v1.2,v1.3", "pri=150", nullptr);
    NmosNode::RegistryCandidate c;
    TestableNmosNode::parse_txt_into_candidate(txt, c);
    avahi_string_list_free(txt);

    EXPECT_EQ(c.api_versions.size(), 4);
    EXPECT_EQ(c.api_versions[0], "v1.0");
    EXPECT_EQ(c.api_versions[1], "v1.1");
    EXPECT_EQ(c.api_versions[2], "v1.2");
    EXPECT_EQ(c.api_versions[3], "v1.3");
    EXPECT_EQ(c.priority, 150);
}

//Add a manual call to resolve_cb with event != AVAHI_RESOLVER_FOUND to get free coverage on the else of the if in resolve_cb
TEST_F(NmosNodeTest, AvahiResolveFailure) {
    // Prepare node (no Avahi daemon, set identity directly)
    TestableNmosNode node;
    node.set_network_identity("127.0.0.1", "lo", "aa-bb-cc-dd-ee-ff", "chassis-123");
    // Call resolve_cb with event != AVAHI_RESOLVER_FOUND
    // Create AvahiServiceResolver* to pass in, as it needs to be a real object for else case
    struct DummyAvahiServiceResolver { int dummy; } dummy_resolver;
    AvahiServiceResolver* r = reinterpret_cast<AvahiServiceResolver*>(&dummy_resolver);
    node.resolve_cb(/*r*/r,
                       /*iface*/AVAHI_IF_UNSPEC,
                       /*proto*/AVAHI_PROTO_UNSPEC,
                       /*event*/AVAHI_RESOLVER_FAILURE,
                       /*name*/"reg-1",
                       /*type*/"_nmos-registration._tcp",
                       /*domain*/"local",
                       /*host_name*/nullptr,
                       /*address*/nullptr,
                       /*port*/0,
                       /*txt*/nullptr,
                       /*flags*/(AvahiLookupResultFlags)0,
                       /*userdata*/&node);
}

TEST_F(NmosNodeTest, AvahiResolveMultipleCandidatesPrio) {
    // Prepare node (no Avahi daemon, set identity directly)
    TestableNmosNode node;
    node.set_network_identity("127.0.0.1", "lo", "aa-bb-cc-dd-ee-ff", "chassis-123");
    // Call resolve_cb with event == AVAHI_RESOLVER_FOUND and different candidates'

    // Candidate 1: priority 50, supports v1.3
    AvahiStringList* txt1 = avahi_string_list_new("api_ver=v1.3", "pri=150", nullptr);
    node.resolve_cb(/*r*/nullptr,
                       /*iface*/AVAHI_IF_UNSPEC,
                       /*proto*/AVAHI_PROTO_UNSPEC,
                       /*event*/AVAHI_RESOLVER_FOUND,
                       /*name*/"reg-1",
                       /*type*/"_nmos-registration._tcp",
                       /*domain*/"local",
                       /*host_name*/"reg-1.local",
                       /*address*/nullptr,
                       /*port*/3210,
                       /*txt*/txt1,
                       /*flags*/(AvahiLookupResultFlags)0,
                       /*userdata*/&node);
    avahi_string_list_free(txt1);
    
    // Candidate 2: priority 100, supports v1.3
    AvahiStringList* txt2 = avahi_string_list_new("api_ver=v1.3", "pri=100", nullptr);
    node.resolve_cb(/*r*/nullptr,
                       /*iface*/AVAHI_IF_UNSPEC,
                       /*proto*/AVAHI_PROTO_UNSPEC,
                       /*event*/AVAHI_RESOLVER_FOUND,
                       /*name*/"reg-2",
                       /*type*/"_nmos-registration._tcp",
                       /*domain*/"local",
                       /*host_name*/"reg-2.local",
                       /*address*/nullptr,
                       /*port*/3211,
                       /*txt*/txt2,
                       /*flags*/(AvahiLookupResultFlags)0,
                       /*userdata*/&node);
    avahi_string_list_free(txt2);

    // Candidate 3: priority 50, supports v1.3
    AvahiStringList* txt3 = avahi_string_list_new("api_ver=v1.3", "pri=50", nullptr);
    node.resolve_cb(/*r*/nullptr,
                       /*iface*/AVAHI_IF_UNSPEC,
                       /*proto*/AVAHI_PROTO_UNSPEC,
                       /*event*/AVAHI_RESOLVER_FOUND,
                       /*name*/"reg-3",
                       /*type*/"_nmos-registration._tcp",
                       /*domain*/"local",
                          /*host_name*/"reg-3.local",
                          /*address*/nullptr,
                            /*port*/3212,
                            /*txt*/txt3,
                            /*flags*/(AvahiLookupResultFlags)0,
                            /*userdata*/&node);
    avahi_string_list_free(txt3);

    // Build base URL from discovered candidates
    auto sel = node.choose_registry_and_build_base("v1.3");
    ASSERT_TRUE(sel.has_value());
    
    // Candidate 3 should be chosen as it has the lowest priority (50) and supports v1.3
    EXPECT_EQ(sel->base, "http://reg-3.local:3212/x-nmos/registration/v1.3");
}

TEST_F(NmosNodeTest, AvahiResolveMultipleCandidatesVer) {
    // Prepare node (no Avahi daemon, set identity directly)
    TestableNmosNode node;
    node.set_network_identity("127.0.0.1", "lo", "aa-bb-cc-dd-ee-ff", "chassis-123");
    // Call resolve_cb with event == AVAHI_RESOLVER_FOUND and different candidates'

    // Candidate 1: priority 50, supports v1.3
    AvahiStringList* txt1 = avahi_string_list_new("api_ver=v1.3", "pri=50", nullptr);
    node.resolve_cb(/*r*/nullptr,
                       /*iface*/AVAHI_IF_UNSPEC,
                       /*proto*/AVAHI_PROTO_UNSPEC,
                       /*event*/AVAHI_RESOLVER_FOUND,
                       /*name*/"reg-1",
                       /*type*/"_nmos-registration._tcp",
                       /*domain*/"local",
                       /*host_name*/"reg-1.local",
                       /*address*/nullptr,
                       /*port*/3210,
                       /*txt*/txt1,
                       /*flags*/(AvahiLookupResultFlags)0,
                       /*userdata*/&node);
    avahi_string_list_free(txt1);
    
    // Candidate 2: priority 100, supports v1.3
    AvahiStringList* txt2 = avahi_string_list_new("api_ver=v1.2", "pri=100", nullptr);
    node.resolve_cb(/*r*/nullptr,
                       /*iface*/AVAHI_IF_UNSPEC,
                       /*proto*/AVAHI_PROTO_UNSPEC,
                       /*event*/AVAHI_RESOLVER_FOUND,
                       /*name*/"reg-2",
                       /*type*/"_nmos-registration._tcp",
                       /*domain*/"local",
                       /*host_name*/"reg-2.local",
                       /*address*/nullptr,
                       /*port*/3211,
                       /*txt*/txt2,
                       /*flags*/(AvahiLookupResultFlags)0,
                       /*userdata*/&node);
    avahi_string_list_free(txt2);

    // Build base URL from discovered candidates
    auto sel = node.choose_registry_and_build_base("v1.2");
    ASSERT_TRUE(sel.has_value());
    
    // Candidate 2 should be chosen as it has a higher priority (100) but supports v1.3
    EXPECT_EQ(sel->base, "http://reg-2.local:3211/x-nmos/registration/v1.2");
}

TEST_F(NmosNodeTest, AvahiResolveMultipleCandidatesNone) {
    // Prepare node (no Avahi daemon, set identity directly)
    TestableNmosNode node;
    node.set_network_identity("127.0.0.1", "lo", "aa-bb-cc-dd-ee-ff", "chassis-123");
    // Call resolve_cb with event == AVAHI_RESOLVER_FOUND and different candidates'

    // Candidate 1: priority 50, supports v1.3
    AvahiStringList* txt1 = avahi_string_list_new("api_ver=v1.3", nullptr);
    node.resolve_cb(/*r*/nullptr,
                       /*iface*/AVAHI_IF_UNSPEC,
                       /*proto*/AVAHI_PROTO_UNSPEC,
                       /*event*/AVAHI_RESOLVER_FOUND,
                       /*name*/"reg-1",
                       /*type*/"_nmos-registration._tcp",
                       /*domain*/"local",
                       /*host_name*/"reg-1.local",
                       /*address*/nullptr,
                       /*port*/3210,
                       /*txt*/txt1,
                       /*flags*/(AvahiLookupResultFlags)0,
                       /*userdata*/&node);
    avahi_string_list_free(txt1);
    
    // Candidate 2: priority 100, supports v1.3
    AvahiStringList* txt2 = avahi_string_list_new("api_ver=v1.3", nullptr);
    node.resolve_cb(/*r*/nullptr,
                       /*iface*/AVAHI_IF_UNSPEC,
                       /*proto*/AVAHI_PROTO_UNSPEC,
                       /*event*/AVAHI_RESOLVER_FOUND,
                       /*name*/"reg-2",
                       /*type*/"_nmos-registration._tcp",
                       /*domain*/"local",
                       /*host_name*/"reg-2.local",
                       /*address*/nullptr,
                       /*port*/3211,
                       /*txt*/txt2,
                       /*flags*/(AvahiLookupResultFlags)0,
                       /*userdata*/&node);
    avahi_string_list_free(txt2);

    // Build base URL from discovered candidates
    auto sel = node.choose_registry_and_build_base("v1.2");
    ASSERT_FALSE(sel.has_value());
}