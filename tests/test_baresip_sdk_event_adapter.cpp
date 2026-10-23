#include "../src/baresip_sdk_event_adapter.h"
#include "../src/sip_event_projection.h"
#include <cstdint>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

static bevent_h* g_callback = nullptr;
static void* g_context = nullptr;
static bool g_fail_registration = false;
static int g_registered = 0;
static int g_unregistered = 0;
// Deterministic fake-SDK callback pause inside bevent_get_ua().
static std::mutex g_pause_mutex;
static std::condition_variable g_pause_cv;
static bool g_pause_callback = false;
static bool g_callback_entered_pause = false;
static bool g_release_callback = false;
static std::atomic<bool> g_watch_unregister{false};
static std::atomic<bool> g_unregistered_during_test{false};
extern "C" int bevent_register(bevent_h* handler, void* arg) {
    if (g_fail_registration) return 22;
    g_callback = handler; g_context = arg;
    ++g_registered;
    return 0;
}
extern "C" void bevent_unregister(bevent_h* handler) {
    if (g_callback == handler) { g_callback = nullptr; g_context = nullptr; }
    ++g_unregistered;
    if (g_watch_unregister.load()) {
        g_unregistered_during_test = true;
        g_pause_cv.notify_all();
    }
}
extern "C" struct ua* bevent_get_ua(const struct bevent* event) {
    {
        std::unique_lock<std::mutex> lock(g_pause_mutex);
        if (g_pause_callback) {
            g_callback_entered_pause = true;
            g_pause_cv.notify_all();
            g_pause_cv.wait(lock, [] { return g_release_callback; });
        }
    }
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
    BaresipCallbackLifecycle lifecycle(mailbox);
    BaresipSdkEventAdapter adapter(lifecycle);
    BaresipEventMailbox competing_mailbox;
    BaresipCallbackLifecycle competing_lifecycle(competing_mailbox);
    BaresipSdkEventAdapter competing(competing_lifecycle);
    ua account{1}, other{2};
    call c1{&account, 1}, c2{&account, 2}, alien{&other, 3};
    bevent ua_event{&account, nullptr};
    bevent other_event{&other, nullptr};
    bevent one{&account, &c1}, two{&account, &c2}, unrelated{&other, &alien};

    if (adapter.start(nullptr) || adapter.active()) return 1;
    g_fail_registration = true;
    if (adapter.start(&account) || mailbox.drain().session != 0) return 2;
    g_fail_registration = false;
    // After failed registration, a different adapter must acquire the lease.
    if (!competing.start(&other) || !competing.active()) return 27;
    if (adapter.start(&account) || !competing.active()) return 28;
    competing.stop();
    if (competing.active() || g_callback) return 29;
    if (!adapter.start(&account) || !adapter.active() || adapter.start(&other)) return 3;
    if (competing.start(&other) || competing_mailbox.drain().session) return 30;
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
    if (adapter.active() || mailbox.drain().session || mailbox.drain().size) return 10;
    // Even after a fail-closed callback the SDK registration still exists
    // until stop() unregisters it; another owner must remain excluded.
    if (competing.start(&other)) return 35;
    adapter.stop();
    if (g_callback || adapter.active()) return 11;
    if (!adapter.start(&account) || adapter.session() <= a) return 12;
    emit(BEVENT_REGISTERING, &ua_event);
    emit(BEVENT_REGISTER_OK, &ua_event);
    emit(BEVENT_CALL_OUTGOING, &one);
    emit(BEVENT_CALL_CLOSED, &one);
    // Pointer reuse after closure in SAME generation is conservatively rejected.
    emit(BEVENT_CALL_INCOMING, &one);
    if (adapter.active() || mailbox.drain().session) return 13;
    adapter.stop();
    if (!adapter.start(&account)) return 14;

    // Baresip shutdown is scoped to a UA (ua_destroy emits it).
    // A different account shutting down must not affect our session.
    emit(BEVENT_SHUTDOWN, &other_event);
    if (!adapter.active()) return 15;
    emit(BEVENT_REGISTERING, &ua_event);
    if (mailbox.drain().size != 1) return 16;
    emit(BEVENT_SHUTDOWN, &ua_event);
    if (adapter.active() || mailbox.drain().session || mailbox.drain().session) return 17;
    adapter.stop();

    // A failure on another UA is also ignored.
    if (!adapter.start(&account)) return 18;
    emit(BEVENT_AUDIO_ERROR, &other_event);
    if (!adapter.active()) return 19;
    emit(BEVENT_AUDIO_ERROR, &ua_event);
    if (adapter.active() || mailbox.drain().session) return 20;
    adapter.stop();

    if (!adapter.start(&account)) return 21;
    emit(BEVENT_SIPSESS_FAILED, &other_event);
    if (!adapter.active()) return 22;
    emit(BEVENT_SIPSESS_FAILED, &ua_event);
    if (adapter.active()) return 23;
    adapter.stop();

    // BEVENT_EXIT is a global event and requires no associated UA.
    if (!adapter.start(&account)) return 24;
    emit(BEVENT_EXIT, nullptr);
    if (adapter.active() || mailbox.drain().session || mailbox.drain().session) return 25;
    adapter.stop();
    if (g_callback != nullptr || g_registered != g_unregistered) return 26;
    // A second adapter is permitted after the original calls stop().
    if (!competing.start(&other) || !competing.active()) return 31;
    if (adapter.start(&account)) return 32;
    emit(BEVENT_REGISTERING, &other_event);
    emit(BEVENT_REGISTER_OK, &other_event);
    auto second_batch = competing_mailbox.drain();
    if (second_batch.size != 2 || second_batch.session == 0) return 33;
    competing.stop();
    if (g_callback || g_registered != g_unregistered) return 34;

    // Simulate a callback remaining inside the fake SDK while stop() runs.
    // The stop thread must unregister but may NOT finish until the callback
    // releases its epoch lease. This guards UA/call object lifetime.
    if (!adapter.start(&account) || !adapter.active()) return 36;
    {
        std::lock_guard<std::mutex> lock(g_pause_mutex);
        g_pause_callback = true;
        g_callback_entered_pause = false;
        g_release_callback = false;
    }
    g_unregistered_during_test = false;
    g_watch_unregister = true;
    auto* saved_callback = g_callback;
    void* saved_context = g_context;
    std::thread native_callback([&] {
        saved_callback(BEVENT_REGISTERING, &ua_event, saved_context);
    });
    {
        std::unique_lock<std::mutex> lock(g_pause_mutex);
        g_pause_cv.wait(lock, [] { return g_callback_entered_pause; });
    }
    std::atomic<bool> stop_finished{false};
    std::thread stop_worker([&] {
        adapter.stop();
        stop_finished = true;
        g_pause_cv.notify_all();
    });
    bool observed_unregister = false;
    bool returned_early = false;
    {
        std::unique_lock<std::mutex> lock(g_pause_mutex);
        observed_unregister = g_pause_cv.wait_for(
            lock, std::chrono::seconds(3),
            [] { return g_unregistered_during_test.load(); });
        // Explicitly hold the callback while giving stop a chance to return.
        // If it returns before release, the whole-callback lease is broken.
        if (observed_unregister) {
            (void)g_pause_cv.wait_for(lock, std::chrono::milliseconds(60),
                                      [&] { return stop_finished.load(); });
            returned_early = stop_finished.load();
        }
        g_release_callback = true;
        g_pause_cv.notify_all();
    }
    native_callback.join();
    stop_worker.join();
    g_watch_unregister = false;
    {
        std::lock_guard<std::mutex> lock(g_pause_mutex);
        g_pause_callback = false;
    }
    if (!observed_unregister) return 37;
    if (returned_early) return 38;
    if (!stop_finished || g_callback || g_registered != g_unregistered) return 39;
    if (!adapter.start(&account) || !adapter.active()) return 40;
    adapter.stop();
    if (g_callback || g_registered != g_unregistered) return 41;
    return 0;
}
