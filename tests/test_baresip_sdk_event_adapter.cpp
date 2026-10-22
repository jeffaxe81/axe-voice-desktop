#include "../src/baresip_sdk_event_adapter.h"
#include "../src/sip_event_projection.h"
#include <cstdint>

static bevent_h* g_callback = nullptr;
static void* g_context = nullptr;
static bool g_fail_registration = false;
static int g_registered = 0;
static int g_unregistered = 0;
extern "C" int bevent_register(bevent_h* handler, void* arg) {
    if (g_fail_registration) return 22;
    g_callback = handler; g_context = arg;
    ++g_registered;
    return 0;
}
extern "C" void bevent_unregister(bevent_h* handler) {
    if (g_callback == handler) { g_callback = nullptr; g_context = nullptr; }
    ++g_unregistered;
}
extern "C" struct ua* bevent_get_ua(const struct bevent* event) {
    return event ? event->owner : nullptr;
}
extern "C" struct call* bevent_get_call(const struct bevent* event) {
    return event ? event->active_call : nullptr;
}
static void emit(bevent_ev kind, bevent* event) {
    if (g_callback) g_callback(kind, event, g_context);
}

int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    BaresipCallbackIngress ingress(mailbox);
    BaresipSdkEventAdapter adapter(ingress);
    ua account{1}, other{2};
    call c1{&account, 1}, c2{&account, 2}, alien{&other, 3};
    bevent ua_event{&account, nullptr};
    bevent other_event{&other, nullptr};
    bevent one{&account, &c1}, two{&account, &c2}, unrelated{&other, &alien};

    if (adapter.start(nullptr) || adapter.active()) return 1;
    g_fail_registration = true;
    if (adapter.start(&account) || ingress.session() != 0) return 2;
    g_fail_registration = false;
    if (!adapter.start(&account) || !adapter.active() || adapter.start(&other)) return 3;
    const auto a = adapter.session();
    if (a == 0) return 4;
    emit(BEVENT_REGISTERING, &other_event);
    emit(BEVENT_CALL_INCOMING, &unrelated);
    if (mailbox.drain().size) return 5; // unrelated account isolation
    emit(BEVENT_REGISTERING, &ua_event);
    emit(BEVENT_REGISTER_OK, &ua_event);
    auto first = mailbox.drain();
    if (first.size != 2 || first.events[0].kind != BaresipEventKind::Registering ||
        first.events[1].session != a) return 6;
    SipEventProjection projection;
    auto state = projection.observe(RuntimeProcess::Running,
                                    SipEvidence::VerifiedNativeApi, first);
    if (state.sip != SipStatus::Registered) return 7;
    emit(BEVENT_CALL_INCOMING, &one);
    emit(BEVENT_CALL_PROGRESS, &one); // unrecognized, should not create events
    emit(BEVENT_CALL_ESTABLISHED, &one);
    emit(BEVENT_CALL_CLOSED, &one);
    emit(BEVENT_CALL_OUTGOING, &two);
    auto batch = mailbox.drain();
    if (batch.size != 4 || batch.events[0].call_id != 1 ||
        batch.events[1].call_id != 1 || batch.events[2].call_id != 1 ||
        batch.events[3].call_id != 2) return 8;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, batch);
    if (state.sip != SipStatus::Calling || state.call_id != 2) return 9;
    // Wrong old call closure must revoke the whole session, not the new call.
    emit(BEVENT_CALL_CLOSED, &one);
    if (adapter.active() || ingress.session() || mailbox.drain().size) return 10;
    adapter.stop();
    if (g_callback || adapter.active()) return 11;
    if (!adapter.start(&account) || adapter.session() <= a) return 12;
    emit(BEVENT_REGISTERING, &ua_event);
    emit(BEVENT_REGISTER_OK, &ua_event);
    emit(BEVENT_CALL_OUTGOING, &one);
    emit(BEVENT_CALL_CLOSED, &one);
    // Pointer reuse after closure in SAME generation is conservatively rejected.
    emit(BEVENT_CALL_INCOMING, &one);
    if (adapter.active() || ingress.session()) return 13;
    adapter.stop();
    if (!adapter.start(&account)) return 14;
    emit(BEVENT_SHUTDOWN, nullptr);
    auto lost = mailbox.drain();
    if (lost.size != 1 || lost.events[0].kind != BaresipEventKind::TransportLost) return 15;
    adapter.stop();
    if (g_callback != nullptr || g_registered != g_unregistered) return 16;
    return 0;
}
