#include "../src/baresip_sdk_event_adapter.h"
#include <cstdint>
#include <atomic>
#include <thread>

// This SDK fake intentionally models the upstream bevent registry's
// handler-pointer uniqueness: replacing a handler would steal callbacks.
static bevent_h* g_handler = nullptr;
static void* g_context = nullptr;
static int g_attempts = 0, g_registered = 0, g_unregistered = 0;
static bool g_fail_registration = false;
extern "C" int bevent_register(bevent_h* handler, void* context) {
    ++g_attempts;
    if (g_fail_registration) return 22;
    if (!handler || g_handler) return 23;
    g_handler = handler;
    g_context = context;
    ++g_registered;
    return 0;
}
extern "C" void bevent_unregister(bevent_h* handler) {
    if (handler == g_handler) {
        g_handler = nullptr;
        g_context = nullptr;
        ++g_unregistered;
    }
}
extern "C" ua* bevent_get_ua(const bevent* ev) {
    return ev ? ev->owner : nullptr;
}
extern "C" call* bevent_get_call(const bevent* ev) {
    return ev ? ev->active_call : nullptr;
}

extern "C" bool ua_isregistered(const struct ua* selected) {
    return selected && selected->test_id == 1;
}

int main() {
    using namespace axv;
    BaresipEventMailbox qa, qb;
    BaresipCallbackLifecycle la(qa), lb(qb);
    BaresipSdkEventAdapter first(la), second(lb);
    ua account{1};
    bevent ev{&account, nullptr};

    if (!first.start(&account) || !first.active()) return 1;
    const auto a = first.session();
    if (!a || !g_handler || g_registered != 1) return 2;
    auto handler = g_handler;
    auto context = g_context;
    if (second.start(&account) || second.active() ||
        g_attempts != 1 || g_handler != handler || g_context != context)
        return 3;
    // Stopping a rejected second adapter must NOT remove first's handler.
    second.stop();
    if (!first.active() || g_handler != handler ||
        g_registered != 1 || g_unregistered != 0) return 4;
    handler(BEVENT_REGISTERING, &ev, context);
    handler(BEVENT_REGISTER_OK, &ev, context);
    auto batch = qa.drain();
    if (batch.session != a || batch.size != 2 || qb.drain().session)
        return 5;

    // A fail-closed callback does not release global handler ownership
    // until the owning SDK event loop explicitly unregisters/stops.
    handler(BEVENT_EXIT, nullptr, context);
    if (first.active() || first.session()) return 6;
    if (second.start(&account) || g_attempts != 1) return 7;
    first.stop();
    if (g_handler || g_unregistered != 1) return 8;
    if (!second.start(&account) || !second.active() ||
        g_registered != 2) return 9;
    if (first.start(&account) || g_attempts != 2) return 10;
    second.stop();

    // An SDK registration error must relinquish the global claim and
    // invalidate the refused session before another owner can start.
    g_fail_registration = true;
    if (first.start(&account) || first.active() || first.session() ||
        g_handler) return 11;
    g_fail_registration = false;
    if (!second.start(&account) || !second.active()) return 12;
    second.stop();
    if (g_registered != g_unregistered) return 13;

    // Alternating owners must be supported across many clean restarts.
    for (int n = 0; n < 64; ++n) {
        auto& owner = n % 2 ? second : first;
        auto& other = n % 2 ? first : second;
        if (!owner.start(&account) || other.start(&account) ||
            !owner.active()) return 14;
        owner.stop();
        if (owner.active() || g_handler) return 15;
    }
    if (g_handler || g_context || g_registered != g_unregistered)
        return 16;

    // Parallel contenders exercise the atomic claim before touching the
    // process-level event registry. The SDK fake is reached by the winner only.
    std::atomic<unsigned> ready{0};
    std::atomic<bool> go{false};
    bool first_won = false, second_won = false;
    std::thread t1([&] {
        ++ready;
        while (!go.load()) std::this_thread::yield();
        first_won = first.start(&account);
    });
    std::thread t2([&] {
        ++ready;
        while (!go.load()) std::this_thread::yield();
        second_won = second.start(&account);
    });
    while (ready.load() != 2) std::this_thread::yield();
    go.store(true);
    t1.join();
    t2.join();
    if (first_won == second_won) return 17;
    if (!g_handler || g_registered != g_unregistered + 1) return 18;
    if (first_won) first.stop();
    else second.stop();
    if (g_handler || g_context || g_registered != g_unregistered)
        return 19;
    return 0;
}
