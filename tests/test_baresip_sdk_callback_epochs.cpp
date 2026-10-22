#include "../src/baresip_sdk_event_adapter.h"
#include <cstdint>

static bevent_h* callback = nullptr;
static void* context = nullptr;
static int registered = 0, unregistered = 0;
extern "C" int bevent_register(bevent_h* fn, void* arg) {
    callback = fn; context = arg; ++registered; return 0;
}
extern "C" void bevent_unregister(bevent_h* fn) {
    if (callback == fn) { callback = nullptr; context = nullptr; }
    ++unregistered;
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
    call native_call{&account,1};
    bevent owner{&account,nullptr}, c{&account,&native_call};
    if (!adapter.start(&account)) return 1;
    auto first = adapter.session();
    auto old_callback = callback; void* old_context = context;
    old_callback(BEVENT_REGISTERING,&owner,old_context);
    old_callback(BEVENT_REGISTER_OK,&owner,old_context);
    if (mailbox.drain().size != 2) return 2;
    adapter.stop();
    if (mailbox.drain().session) return 3;
    if (!adapter.start(&account) || adapter.session() <= first) return 4;
    const auto second = adapter.session();
    // A callback captured before unregister must not mutate the new session.
    old_callback(BEVENT_REGISTER_OK,&owner,old_context);
    old_callback(BEVENT_CALL_INCOMING,&c,old_context);
    auto stale = mailbox.drain();
    if (stale.session != second || stale.size || !adapter.active()) return 5;
    callback(BEVENT_REGISTERING,&owner,context);
    callback(BEVENT_REGISTER_OK,&owner,context);
    if (mailbox.drain().size != 2) return 6;
    callback(BEVENT_CALL_INCOMING,&c,context);
    auto incoming = mailbox.drain();
    if (incoming.size != 1 || incoming.events[0].call_id != 1) return 7;
    old_callback(BEVENT_CALL_CLOSED,&c,old_context);
    if (!adapter.active() || mailbox.drain().size) return 8;
    callback(BEVENT_CALL_CLOSED,&c,context);
    auto closed = mailbox.drain();
    if (closed.size != 1 || closed.events[0].call_id != 1) return 9;
    adapter.stop();
    if (callback || mailbox.drain().session) return 10;
    auto previous = second;
    for (int i=0;i<100;++i) {
        if (!adapter.start(&account) || adapter.session() <= previous) return 11;
        previous = adapter.session();
        adapter.stop();
    }
    if (registered != unregistered) return 12;
    return 0;
}
