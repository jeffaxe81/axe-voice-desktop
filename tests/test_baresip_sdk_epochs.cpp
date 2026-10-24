#include "../src/baresip_sdk_event_adapter.h"
#include <cstdint>

static bevent_h* g_handler = nullptr;
static void* g_context = nullptr;
static int g_registered = 0;
static int g_unregistered = 0;
extern "C" int bevent_register(bevent_h* cb, void* arg) {
    g_handler = cb; g_context = arg; ++g_registered; return 0;
}
extern "C" void bevent_unregister(bevent_h* cb) {
    if (g_handler == cb) { g_handler = nullptr; g_context = nullptr; }
    ++g_unregistered;
}
extern "C" ua* bevent_get_ua(const bevent* event) {
    return event ? event->owner : nullptr;
}
extern "C" call* bevent_get_call(const bevent* event) {
    return event ? event->active_call : nullptr;
}
int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    BaresipCallbackLifecycle lifecycle(mailbox);
    BaresipSdkEventAdapter adapter(lifecycle);
    ua account{1};
    call first_call{&account,1};
    bevent identity{&account,nullptr}, call_event{&account,&first_call};
    if (!adapter.start(&account)) return 1;
    const auto first = adapter.session();
    auto old_handler = g_handler;
    void* old_context = g_context;
    old_handler(BEVENT_REGISTERING, &identity, old_context);
    old_handler(BEVENT_REGISTER_OK, &identity, old_context);
    if (mailbox.drain().size != 2) return 2;
    adapter.stop();
    if (mailbox.drain().session) return 3;
    if (!adapter.start(&account) || adapter.session() <= first) return 4;
    const auto second = adapter.session();
    old_handler(BEVENT_REGISTER_OK, &identity, old_context);
    old_handler(BEVENT_CALL_INCOMING, &call_event, old_context);
    const auto old_events = mailbox.drain();
    if (old_events.size || old_events.session != second || !adapter.active()) return 5;
    g_handler(BEVENT_REGISTERING, &identity, g_context);
    g_handler(BEVENT_REGISTER_OK, &identity, g_context);
    if (mailbox.drain().size != 2) return 6;
    g_handler(BEVENT_CALL_INCOMING, &call_event, g_context);
    auto new_events = mailbox.drain();
    if (new_events.size != 1 || new_events.events[0].call_id != 1) return 7;
    old_handler(BEVENT_CALL_CLOSED, &call_event, old_context);
    if (!adapter.active() || mailbox.drain().size) return 8;
    g_handler(BEVENT_CALL_CLOSED, &call_event, g_context);
    if (mailbox.drain().size != 1) return 9;
    adapter.stop();
    if (g_handler || mailbox.drain().session ||
        g_registered != g_unregistered) return 10;
    std::uint64_t previous=second;
    for(int i=0;i<100;++i) {
        if(!adapter.start(&account) || adapter.session()<=previous) return 11;
        previous=adapter.session();
        adapter.stop();
    }
    if(g_registered!=g_unregistered) return 12;
    return 0;
}
