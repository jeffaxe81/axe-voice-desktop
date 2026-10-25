#include "../src/baresip_sdk_call_port.h"
#include <atomic>
#include <string>
#include <thread>

static bevent_h* g_handler = nullptr;
static void* g_context = nullptr;
static call* g_next_call = nullptr;
static int g_dials = 0, g_answers = 0, g_hangups = 0;
static bool g_connect_fail = false;
static std::string g_last_uri;

extern "C" int bevent_register(bevent_h* handler, void* arg) {
    if (!handler || g_handler) return 22;
    g_handler = handler;
    g_context = arg;
    return 0;
}
extern "C" void bevent_unregister(bevent_h* handler) {
    if (handler == g_handler) {
        g_handler = nullptr;
        g_context = nullptr;
    }
}
extern "C" ua* bevent_get_ua(const bevent* ev) {
    return ev ? ev->owner : nullptr;
}
extern "C" call* bevent_get_call(const bevent* ev) {
    return ev ? ev->active_call : nullptr;
}
static void emit(bevent_ev type, ua* account, call* active = nullptr) {
    bevent event{account, active};
    if (g_handler) g_handler(type, &event, g_context);
}
extern "C" int ua_connect(ua* account, call** out, const char* from,
                            const char* destination, enum vidmode mode) {
    if (!account || !destination || from || mode != VIDMODE_OFF)
        return 22;
    ++g_dials;
    g_last_uri = destination;
    if (g_connect_fail) return 5;
    if (!g_next_call || g_next_call->owner != account) return 6;
    if (out) *out = g_next_call;
    emit(BEVENT_CALL_OUTGOING, account, g_next_call);
    return 0;
}
extern "C" int ua_answer(ua* account, call* current, enum vidmode mode) {
    if (!account || !current || current->owner != account ||
        mode != VIDMODE_OFF) return 22;
    ++g_answers;
    emit(BEVENT_CALL_ESTABLISHED, account, current);
    return 0;
}
extern "C" void ua_hangup(ua* account, call* current,
                            unsigned short code, const char* reason) {
    if (!account || !current || current->owner != account ||
        code != 0 || reason) return;
    ++g_hangups;
    emit(BEVENT_CALL_CLOSED, account, current);
}

int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    BaresipCallbackLifecycle lifecycle(mailbox);
    BaresipSdkEventAdapter adapter(lifecycle);
    BaresipSdkCallPort prestart(adapter);
    ua account{1};
    call first{&account, 11}, second{&account, 22}, third{&account, 33};

    if (prestart.originate(L"sip:alice@example.org") ||
        prestart.answer(1) || prestart.hangup(1)) return 1;
    if (!adapter.start(&account)) return 2;
    BaresipSdkCallPort port(adapter);
    if (port.originate(L"sip:alice@example.org") || g_dials) return 3;
    if (prestart.originate(L"sip:alice@example.org")) return 4;
    emit(BEVENT_REGISTERING, &account);
    emit(BEVENT_REGISTER_OK, &account);
    if (mailbox.drain().size != 2) return 5;
    if (port.originate(L"1001") ||
        port.originate(L"sip:alice@example.org\n") ||
        port.originate(L"sip:álîce@example.org") ||
        port.originate(L"sip:alice@example.org\x0000evil"))
        return 6;
    if (g_dials) return 7;

    // Calls are bound to the exact opaque token returned by the mailbox.
    g_next_call = &first;
    if (!port.originate(L"sip:alice@example.org") ||
        g_last_uri != "sip:alice@example.org" || g_dials != 1) return 8;
    auto outgoing = mailbox.drain();
    if (outgoing.size != 1 ||
        outgoing.events[0].kind != BaresipEventKind::CallOutgoing ||
        outgoing.events[0].call_id != 1) return 9;
    if (port.answer(1) || port.hangup(0) || port.hangup(2) ||
        g_answers || g_hangups) return 10;
    if (!port.hangup(1) || g_hangups != 1 || port.hangup(1)) return 11;
    auto closed = mailbox.drain();
    if (closed.size != 1 || closed.events[0].call_id != 1) return 12;

    // New incoming call gets a fresh token. A stale token cannot hang it up.
    emit(BEVENT_CALL_INCOMING, &account, &second);
    auto incoming = mailbox.drain();
    if (incoming.size != 1 || incoming.events[0].call_id != 2)
        return 13;
    if (port.answer(1) || port.hangup(1)) return 14;
    if (!port.answer(2) || g_answers != 1 || port.answer(2)) return 15;
    auto established = mailbox.drain();
    if (established.size != 1 ||
        established.events[0].kind != BaresipEventKind::CallEstablished ||
        established.events[0].call_id != 2) return 16;
    if (!port.hangup(2) || g_hangups != 2 || port.hangup(2))
        return 17;
    mailbox.drain();

    // Invoke from another thread: no raw native command is dispatched.
    std::atomic<bool> cross_thread_accepted{false};
    std::thread outsider([&] {
        if (port.originate(L"sip:alice@example.org"))
            cross_thread_accepted = true;
    });
    outsider.join();
    if (cross_thread_accepted || g_dials != 1) return 18;

    emit(BEVENT_UNREGISTERING, &account);
    mailbox.drain();
    if (port.originate(L"sip:alice@example.org") || g_dials != 1)
        return 19;

    adapter.stop();
    if (port.originate(L"sip:alice@example.org") || port.hangup(2))
        return 20;
    if (!adapter.start(&account)) return 21;
    BaresipSdkCallPort fresh(adapter);
    emit(BEVENT_REGISTERING, &account);
    emit(BEVENT_REGISTER_OK, &account);
    mailbox.drain();
    if (port.originate(L"sip:alice@example.org")) return 22;
    g_next_call = &third;
    g_connect_fail = true;
    if (fresh.originate(L"sip:alice@example.org") || g_dials != 2) return 23;
    g_connect_fail = false;
    if (!fresh.originate(L"sips:alice@example.org") ||
        g_last_uri != "sips:alice@example.org") return 24;
    auto next = mailbox.drain();
    if (next.size != 1 || next.events[0].call_id != 1) return 25;
    if (fresh.hangup(2) || !fresh.hangup(1) || g_hangups != 3) return 26;
    adapter.stop();
    if (g_handler || g_context) return 27;
    return 0;
}

// Synthetic registrar confirmation for this focused ownership/call policy fixture.
extern "C" bool ua_isregistered(const struct ua* selected) { return selected != nullptr; }
