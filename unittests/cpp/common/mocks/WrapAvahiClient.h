#include <string>
#include <cstring>
#include <avahi-client/lookup.h>
#include <avahi-common/defs.h>
#include <avahi-common/address.h>
#include <avahi-common/simple-watch.h>

struct AvahiTestControl {
    int err;
    AvahiSimplePoll* simple_poll_new_return;
    AvahiClient* client_new_return;
    AvahiClientState client_state;
    AvahiServiceBrowser* service_browser_new_return;
    AvahiBrowserEvent browse_event;
    bool discovered_service;
    std::shared_ptr<AvahiAddress> avahi_address;
    bool has_iface;
    int simple_poll_quit;
    std::string expected_type;

    void reset() {
        err = 1;
        simple_poll_new_return = (AvahiSimplePoll*)0x1;
        client_new_return = (AvahiClient*)0x1;
        client_state = AVAHI_CLIENT_S_RUNNING;
        service_browser_new_return = (AvahiServiceBrowser*)0x1;
        browse_event = AVAHI_BROWSER_NEW;
        discovered_service = false;
        avahi_address = std::make_shared<AvahiAddress>();
        simple_poll_quit = 0;
        has_iface = true;
        expected_type = "_nmos-registration._tcp";
    }
};

// Declare the global as extern so it can be shared across translation units
extern AvahiTestControl g_avahi_test_control;

extern "C" {

int __wrap_avahi_client_errno(AvahiClient *c) {
    (void)c;
    return g_avahi_test_control.err;
}

AvahiSimplePoll* __wrap_avahi_simple_poll_new() {
    return g_avahi_test_control.simple_poll_new_return; // any non-null
}
void __wrap_avahi_simple_poll_free(AvahiSimplePoll *s) {
    (void)s; // no-op
}
int __wrap_avahi_simple_poll_loop(AvahiSimplePoll *s) {
    (void)s; // no-op
    return 0;
}
void __wrap_avahi_simple_poll_quit(AvahiSimplePoll *s) {
    (void)s; // no-op
    g_avahi_test_control.simple_poll_quit++;
}

AvahiClient* __wrap_avahi_client_new(
    const AvahiPoll *poll_api,
    AvahiClientFlags flags,
    AvahiClientCallback callback,
    void *userdata,
    int *error) 
{
    (void)poll_api; (void)flags; (void)callback; (void)userdata; (void)error;
    callback(g_avahi_test_control.client_new_return,
        g_avahi_test_control.client_state,
        userdata);
    return g_avahi_test_control.client_new_return; // any non-null
}
void __wrap_avahi_client_free(AvahiClient *c) {
    (void)c; // no-op
}

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
    if (!type || std::strcmp(type, g_avahi_test_control.expected_type.c_str()) != 0) {
        //create a dummy AvahiClient to get errno
        // Simulate failure to create a resolver for the wrong type
        return nullptr; // your browse_cb should log and NOT add a candidate
    }
    if (g_avahi_test_control.has_iface)
    {
        g_avahi_test_control.avahi_address->proto = AVAHI_PROTO_INET;
        g_avahi_test_control.avahi_address->data.ipv4.address = htonl(0x7f000001);
    } //

    cb(/*resolver*/NULL,
       interface,
       protocol,
       AVAHI_RESOLVER_FOUND,
       name,
       type,
       domain,
       "127.0.0.1",          // host_name
       g_avahi_test_control.avahi_address.get(), // address
       3210,                  // port to match the fake HTTP server
       /*txt*/avahi_string_list_new("api_ver=v1.3", nullptr),           // no TXT → parse_txt loop is skipped
       /*lookup flags*/(AvahiLookupResultFlags)0,
       userdata);
    return (AvahiServiceResolver*)0x1; // any non-null
}

void __wrap_avahi_service_resolver_free(AvahiServiceResolver *r) {
    (void)r; // no-op
}

AvahiServiceBrowser*
__wrap_avahi_service_browser_new(AvahiClient *client,
                                 AvahiIfIndex interface,
                                 AvahiProtocol protocol,
                                 const char *type,
                                 const char *domain,
                                 AvahiLookupFlags flags,
                                 AvahiServiceBrowserCallback cb,
                                 void *userdata) {
    (void)client; (void)interface; (void)protocol; (void)domain; (void)flags; (void)cb; (void)userdata;

    if (!type || std::strcmp(type, "_nmos-registration._tcp") != 0) {
        return nullptr; // simulate browser creation failure for wrong type
    }
    if (g_avahi_test_control.discovered_service) {
        // Simulate a discovered service
        cb(/*browser*/nullptr,
           /*iface*/AVAHI_IF_UNSPEC,
           /*proto*/AVAHI_PROTO_UNSPEC,
           g_avahi_test_control.browse_event,
           /*name*/"reg-1",
           /*type*/type,
           /*domain*/"local",
           /*flags*/(AvahiLookupResultFlags)0,
           userdata);
    }
    return g_avahi_test_control.service_browser_new_return;
}
void __wrap_avahi_service_browser_free(AvahiServiceBrowser *b) {
    (void)b; // no-op
}

} // extern "C"