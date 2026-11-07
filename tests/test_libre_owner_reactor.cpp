#include "../src/baresip_native_libre_reactor.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <thread>
using namespace std::chrono_literals;

// Fake libre reactor: actual UI and owner C++ threads, C API substitutions.
// This test does NOT load a SIP stack or any native SDK DLL.
struct mqueue {
    std::mutex lock;
    std::condition_variable cv;
    std::deque<int> pending;
    mqueue_h* callback=nullptr;
    void* context=nullptr;
};
static mqueue* active=nullptr;  // touched on owner thread only
static tmr* active_timer=nullptr; // touched on owner thread only
static bool cancelled=false;  // owner thread only
static std::thread::id native_owner;
static std::atomic<int> starts{0},closes{0},allocations{0},frees{0};
static std::atomic<bool> fail_init{false},fail_alloc{false},fail_push{false};
static std::atomic<bool> wrong_thread{false};
static std::atomic<bool> break_main{false};

extern "C" int libre_init(void) {
    native_owner=std::this_thread::get_id();
    ++starts;
    cancelled=false;
    return fail_init?11:0;
}
extern "C" void libre_close(void) {
    if (std::this_thread::get_id()!=native_owner) wrong_thread=true;
    ++closes;
}
extern "C" int mqueue_alloc(mqueue** out,mqueue_h* callback,void* arg) {
    if (std::this_thread::get_id()!=native_owner) wrong_thread=true;
    if (fail_alloc) return 12;
    auto* q=new mqueue;
    q->callback=callback;
    q->context=arg;
    *out=q;
    active=q;
    ++allocations;
    return 0;
}
extern "C" int mqueue_push(mqueue* q,int id,void*) {
    if (!q || fail_push) return 13;
    {std::lock_guard<std::mutex> lock(q->lock);q->pending.push_back(id);}
    q->cv.notify_one();
    return 0;
}
extern "C" void* mem_deref(void* ptr) {
    if (std::this_thread::get_id()!=native_owner) wrong_thread=true;
    auto* q=static_cast<mqueue*>(ptr);
    if (q) {
        active=nullptr;
        delete q;
        ++frees;
    }
    return nullptr;
}
extern "C" void tmr_init(tmr* timer) {
    if (std::this_thread::get_id()!=native_owner) wrong_thread=true;
    timer->handler=nullptr;
    timer->arg=nullptr;
    timer->wait_ms=0;
}
extern "C" void tmr_start_dbg(tmr* timer,std::uint64_t delay,
                              tmr_h* handler,void* arg,const char*,int) {
    if (std::this_thread::get_id()!=native_owner) wrong_thread=true;
    timer->handler=handler;
    timer->arg=arg;
    timer->wait_ms=delay;
    active_timer=timer;
}
extern "C" void tmr_cancel(tmr* timer) {
    if (std::this_thread::get_id()!=native_owner) wrong_thread=true;
    timer->handler=nullptr;
    active_timer=nullptr;
}
extern "C" void re_cancel(void) {
    if (std::this_thread::get_id()!=native_owner) wrong_thread=true;
    cancelled=true;
    if (active) active->cv.notify_all();
}
extern "C" int re_main(re_signal_h*) {
    if (std::this_thread::get_id()!=native_owner) wrong_thread=true;
    while(!cancelled) {
        auto* timer=active_timer;
        if (timer && timer->handler) {
            auto* cb=timer->handler;
            void* arg=timer->arg;
            timer->handler=nullptr;
            cb(arg);
        }
        if(cancelled) break;
        int event_id=0;
        {
            std::unique_lock<std::mutex> lock(active->lock);
            (void)active->cv.wait_for(lock,2ms,[&]{
                return !active->pending.empty() || cancelled;
            });
            if(!active->pending.empty()) {
                event_id=active->pending.front();
                active->pending.pop_front();
            }
        }
        if (event_id) active->callback(event_id,nullptr,active->context);
    }
    return break_main?14:0;
}

template <class Predicate>
static bool observed_within(Predicate ready,std::chrono::milliseconds timeout=1000ms) {
    const auto deadline=std::chrono::steady_clock::now()+timeout;
    do {
        if(ready())return true;
        std::this_thread::sleep_for(1ms);
    } while(std::chrono::steady_clock::now()<deadline);
    return ready();
}

static int fail_with_diagnostics(int code) {
    std::fprintf(stderr,
        "libre_owner_reactor failure code=%d starts=%d closes=%d allocations=%d "
        "frees=%d wrong_thread=%d init=%d alloc=%d push=%d main=%d\n",
        code,starts.load(),closes.load(),allocations.load(),frees.load(),
        static_cast<int>(wrong_thread.load()),static_cast<int>(fail_init.load()),
        static_cast<int>(fail_alloc.load()),static_cast<int>(fail_push.load()),
        static_cast<int>(break_main.load()));
    return code;
}

int main() {
    using axv::BaresipNativeLibreReactor;
    using axv::LibreReactorFault;
    std::atomic<unsigned> ticks{0},start_hooks{0},stop_hooks{0};
    std::thread::id first_owner,first_stop;
    BaresipNativeLibreReactor loop(
        [&]{
            if(std::this_thread::get_id()!=native_owner) wrong_thread=true;
            ++ticks;
        },
        [&]{first_owner=std::this_thread::get_id();++start_hooks;},
        [&]{first_stop=std::this_thread::get_id();++stop_hooks;},5ms);
    if(!loop.start() || loop.start() || !loop.wake()) return fail_with_diagnostics(1);
    std::thread gui([&] {for(int i=0;i<8;++i) if(!loop.wake()) wrong_thread=true;});
    gui.join();
    // Wait for an observed owner-thread callback rather than assuming the
    // Windows CI scheduler always runs a timer within 30 milliseconds.
    const bool tick_seen=observed_within([&] {
        return ticks.load()>=1 && loop.snapshot().ticks>=1;
    });
    if(!loop.stop() || loop.wake()) return fail_with_diagnostics(2);
    auto snapshot=loop.snapshot();
    if(!tick_seen || snapshot.running || snapshot.generation!=1 || snapshot.ticks<1 ||
       snapshot.fault!=LibreReactorFault::None || ticks<1 ||
       start_hooks!=1 || stop_hooks!=1 || first_owner!=first_stop ||
       wrong_thread || allocations!=1 || frees!=1 || closes!=1)
       return fail_with_diagnostics(3);

    // After a full join a fresh native queue and timer may be created.
    if(!loop.start() || loop.snapshot().generation!=2) return fail_with_diagnostics(4);
    fail_push=true; // Stop message cannot enqueue: periodic timer is fallback.
    if(!loop.stop()) return fail_with_diagnostics(5);
    fail_push=false;
    if(loop.snapshot().running || allocations!=2 || frees!=2 ||
       closes!=2 || start_hooks!=2 || stop_hooks!=2) return fail_with_diagnostics(6);

    fail_alloc=true;
    if(loop.start() || loop.snapshot().fault!=LibreReactorFault::QueueRejected)
        return fail_with_diagnostics(7);
    if(!loop.stop() || closes!=3) return fail_with_diagnostics(8);
    fail_alloc=false;

    fail_init=true;
    if(loop.start() || loop.snapshot().fault!=LibreReactorFault::InitRejected)
        return fail_with_diagnostics(9);
    if(!loop.stop() || closes!=3) return fail_with_diagnostics(10);
    fail_init=false;

    BaresipNativeLibreReactor throwing([] {throw std::runtime_error("test-only");},
                                      {},{},5ms);
    // start() can return false if the 5ms timer executes the throwing hook
    // before the calling thread reads its ready snapshot. That immediate,
    // correctly reported HookFailed is not a startup failure.
    const bool throwing_started=throwing.start();
    if(!throwing_started &&
       throwing.snapshot().fault!=LibreReactorFault::HookFailed)
        return fail_with_diagnostics(11);
    // Wait for a real callback fault rather than assuming an order between
    // the worker's first tick and the caller's start() return.
    const bool fault_seen=observed_within([&] {
        return throwing.snapshot().fault==LibreReactorFault::HookFailed;
    });
    const bool throwing_stopped=throwing.stop();
    if(!fault_seen || !throwing_stopped ||
       throwing.snapshot().fault!=LibreReactorFault::HookFailed ||
       throwing.snapshot().running || wrong_thread)
        return fail_with_diagnostics(12);

    break_main=true;
    BaresipNativeLibreReactor failing_loop([]{}, {},{},5ms);
    if(!failing_loop.start()) return fail_with_diagnostics(13);
    if(!failing_loop.stop() ||
       failing_loop.snapshot().fault!=LibreReactorFault::MainLoopFailed)
        return fail_with_diagnostics(14);
    break_main=false;
    if(allocations!=frees) return fail_with_diagnostics(15);
    return 0;
}
