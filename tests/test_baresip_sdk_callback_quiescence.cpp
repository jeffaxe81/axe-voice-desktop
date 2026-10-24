#include "../src/baresip_sdk_event_adapter.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
using namespace std::chrono_literals;

static bevent_h* g_handler = nullptr;
static void* g_context = nullptr;
static std::mutex g_mutex;
static std::condition_variable g_cv;
static bool g_pause = false, g_inside = false, g_release = false;
static std::atomic<int> g_registrations{0}, g_unregistrations{0};

extern "C" int bevent_register(bevent_h* fn, void* ctx) {
    if (!fn || g_handler) return 19;
    g_handler = fn; g_context = ctx; ++g_registrations; return 0;
}
extern "C" void bevent_unregister(bevent_h* fn) {
    if (fn == g_handler) { g_handler = nullptr; g_context = nullptr; ++g_unregistrations; }
}
extern "C" ua* bevent_get_ua(const bevent* ev) {
    std::unique_lock<std::mutex> lock(g_mutex);
    if (g_pause) {
        g_inside = true;
        g_cv.notify_all();
        g_cv.wait(lock, [] { return g_release; });
    }
    return ev ? ev->owner : nullptr;
}
extern "C" call* bevent_get_call(const bevent* ev) {
    return ev ? ev->active_call : nullptr;
}
extern "C" bool ua_isregistered(const ua* account) {
    return account && account->test_id == 1;
}

int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    BaresipCallbackLifecycle lifecycle(mailbox);
    BaresipSdkEventAdapter adapter(lifecycle);
    ua account{1};
    bevent event{&account, nullptr};
    if (!adapter.start(&account) || !adapter.active()) return 1;
    const auto previous = adapter.session();
    const auto old_callback = g_handler;
    const auto old_context = g_context;
    if (!previous || !old_callback || !old_context) return 2;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_pause = true;
        g_release = false;
        g_inside = false;
    }
    std::thread callback([&] { old_callback(BEVENT_REGISTERING, &event, old_context); });
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        if (!g_cv.wait_for(lock, 5s, [] { return g_inside; })) {
            g_release = true; lock.unlock(); g_cv.notify_all(); callback.join();
            return 3;
        }
    }
    std::atomic<bool> stopping{false}, stopped{false};
    std::thread shutdown([&] {
        stopping = true;
        adapter.stop();
        stopped = true;
    });
    while (!stopping.load()) std::this_thread::yield();
    std::this_thread::sleep_for(25ms);
    const bool early_stop = stopped.load();
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_release = true;
        g_pause = false;
    }
    g_cv.notify_all();
    callback.join();
    shutdown.join();
    if (early_stop || !stopped || g_handler || mailbox.drain().session ||
        g_registrations != g_unregistrations) return 4;
    // The stale SDK context stays allocated and must not affect a restart.
    if (!adapter.start(&account) || adapter.session() <= previous) return 5;
    old_callback(BEVENT_REGISTER_OK, &event, old_context);
    if (!adapter.active() || mailbox.drain().size) return 6;
    auto current_callback = g_handler;
    auto current_context = g_context;
    current_callback(BEVENT_REGISTERING, &event, current_context);
    current_callback(BEVENT_REGISTER_OK, &event, current_context);
    if (!adapter.native_register_ok_seen() || mailbox.drain().size != 2) return 7;
    adapter.stop();
    if (g_handler || g_registrations != g_unregistrations) return 8;
    return 0;
}
