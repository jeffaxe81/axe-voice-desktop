#include "../src/baresip_sdk_event_adapter.h"
#include "../src/sip_event_projection.h"
#include <cstdint>

static bevent_h* g_callback = nullptr;
static void* g_context = nullptr;
static bool g_fail_registration = false;
static int g_registered = 0;
static int g_unregistered = 0;
static int g_ua_lookups = 0;
static bool g_native_registered = true;
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
    ++g_ua_lookups;
    return event ? event->owner : nullptr;
}
extern "C" struct call* bevent_get_call(const struct bevent* event) {
    return event ? event->active_call : nullptr;
}
extern "C" bool ua_isregistered(const struct ua* selected) {
    return selected && selected->test_id == 1 && g_native_registered;
}
static void emit(bevent_ev kind, bevent* event) {
    if (g_callback) g_callback(kind, event, g_context);
}

int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    BaresipCallbackLifecycle lifecycle(mailbox);
    BaresipSdkEventAdapter adapter(lifecycle);
    ua account{1}, other{2};
    call c1{&account, 1}, c2{&account, 2}, alien{&other, 3};
    bevent ua_event{&account, nullptr};
    bevent other_event{&other, nullptr};
    bevent one{&account, &c1}, two{&account, &c2}, unrelated{&other, &alien};

    if (adapter.start(nullptr) || adapter.active()) return 1;
    g_fail_registration = true;
    if (adapter.start(&account) || adapter.session() != 0) return 2;
    g_fail_registration = false;
    if (!adapter.start(&account) || !adapter.active() || adapter.start(&other)) return 3;
    const auto a = adapter.session();
    if (a == 0 || adapter.native_register_ok_seen()) return 4;
    emit(BEVENT_REGISTERING, &other_event);
    emit(BEVENT_CALL_INCOMING, &unrelated);
    if (mailbox.drain().size) return 5; // unrelated account isolation
    emit(BEVENT_REGISTERING, &ua_event);
    emit(BEVENT_REGISTER_OK, &ua_event);
    auto first = mailbox.drain();
    if (first.size != 2 || first.events[0].kind != BaresipEventKind::Registering ||
        first.events[1].session != a || !adapter.native_register_ok_seen()) return 6;
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
    if (adapter.active() || adapter.session() || mailbox.drain().size) return 10;
    adapter.stop();
    if (g_callback || adapter.active()) return 11;
    if (!adapter.start(&account) || adapter.session() <= a) return 12;
    emit(BEVENT_REGISTERING, &ua_event);
    emit(BEVENT_REGISTER_OK, &ua_event);
    emit(BEVENT_CALL_OUTGOING, &one);
    emit(BEVENT_CALL_CLOSED, &one);
    // Pointer reuse after closure in SAME generation is conservatively rejected.
    emit(BEVENT_CALL_INCOMING, &one);
    if (adapter.active() || adapter.session()) return 13;
    adapter.stop();
    if (!adapter.start(&account)) return 14;

    // Baresip shutdown is scoped to a UA (ua_destroy emits it).
    // A different account shutting down must not affect our session.
    emit(BEVENT_SHUTDOWN, &other_event);
    if (!adapter.active()) return 15;
    emit(BEVENT_REGISTERING, &ua_event);
    if (mailbox.drain().size != 1) return 16;
    emit(BEVENT_SHUTDOWN, &ua_event);
    if (adapter.active() || adapter.session() || mailbox.drain().session) return 17;
    adapter.stop();

    // A failure on another UA is also ignored.
    if (!adapter.start(&account)) return 18;
    emit(BEVENT_AUDIO_ERROR, &other_event);
    if (!adapter.active()) return 19;
    emit(BEVENT_AUDIO_ERROR, &ua_event);
    if (adapter.active() || mailbox.drain().session) return 20;
    adapter.stop();

    if (!adapter.start(&account)) return 21;
    // These events originate from the global SIP-message/media paths; Baresip
    // does not authenticate them as the selected UA's registration/call.
    const auto looked_up_before = g_ua_lookups;
    emit(BEVENT_SIPSESS_FAILED, &other_event);
    emit(BEVENT_SIPSESS_FAILED, &ua_event); // forged UA-like fake payload
    emit(BEVENT_SIPSESS_FAILED, nullptr);
    emit(BEVENT_SIPSESS_CONN, &ua_event);
    emit(BEVENT_CALL_RTPESTAB, &ua_event);
    emit(BEVENT_CALL_MENC, &ua_event);
    emit(BEVENT_CALL_PROGRESS, &ua_event);
    if (!adapter.active() || g_ua_lookups != looked_up_before ||
        mailbox.drain().size != 0) return 22;
    emit(BEVENT_REGISTERING, &ua_event);
    emit(BEVENT_REGISTER_OK, &ua_event);
    if (!adapter.active() || mailbox.drain().size != 2) return 23;
    adapter.stop();

    // A forged or mistimed REGISTER_OK without native registrar approval
    // must revoke the entire session and never allow SIP call controls.
    if (!adapter.start(&account) || adapter.native_register_ok_seen()) return 27;
    emit(BEVENT_REGISTERING, &ua_event);
    g_native_registered = false;
    emit(BEVENT_REGISTER_OK, &ua_event);
    if (adapter.active() || adapter.native_register_ok_seen() ||
        adapter.session() || mailbox.drain().session) return 28;
    adapter.stop();
    g_native_registered = true;

    // BEVENT_EXIT is a global event and requires no associated UA.
    if (!adapter.start(&account)) return 24;
    emit(BEVENT_EXIT, nullptr);
    if (adapter.active() || adapter.session() || mailbox.drain().session) return 25;
    adapter.stop();
    if (g_callback != nullptr || g_registered != g_unregistered) return 26;
    return 0;
}
