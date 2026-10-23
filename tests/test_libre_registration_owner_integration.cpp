#include "../src/baresip_native_registration_host.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
using namespace std::chrono_literals;

// An isolated libre + Baresip double. The AXE GUI remains disconnected.
// re_main owns both mqueue delivery and all Baresip register/unregister calls.
struct mqueue {
    std::mutex mutex;
    std::condition_variable cv;
    std::deque<int> commands;
    mqueue_h* handler=nullptr;
    void* context=nullptr;
};
static mqueue* current_queue=nullptr; // owner thread only
static tmr* current_timer=nullptr;     // owner thread only
static std::thread::id owner_thread;   // owner thread only
static bool cancelled=false;          // owner thread only
static bevent_h* event_handler=nullptr;
static void* event_context=nullptr;
static bool sdk_registered=false;
static std::atomic<bool> wrong_owner{false},emit_ok{false},emit_shutdown{false};
static std::atomic<bool> deny_init{false},deny_push{false};
static std::atomic<unsigned> native_register{0},native_unregister{0},
                             callbacks_added{0},callbacks_removed{0},
                             libre_started{0},libre_closed{0},queues_freed{0};
static ua selected{1};

extern "C" int libre_init(void) {
    owner_thread=std::this_thread::get_id();
    cancelled=false;
    ++libre_started;
    return deny_init?71:0;
}
extern "C" void libre_close(void) {
    if(std::this_thread::get_id()!=owner_thread || event_handler)
        wrong_owner=true;
    ++libre_closed;
}
extern "C" int mqueue_alloc(mqueue** q,mqueue_h* fn,void* ctx) {
    if(std::this_thread::get_id()!=owner_thread) wrong_owner=true;
    auto* ptr=new mqueue;
    ptr->handler=fn;ptr->context=ctx;
    *q=ptr;current_queue=ptr;return 0;
}
extern "C" int mqueue_push(mqueue* q,int id,void*) {
    if(!q || deny_push) return 73;
    {std::lock_guard<std::mutex> lock(q->mutex);q->commands.push_back(id);}
    q->cv.notify_one();return 0;
}
extern "C" void* mem_deref(void* p) {
    if(std::this_thread::get_id()!=owner_thread) wrong_owner=true;
    auto* q=static_cast<mqueue*>(p);
    if(q) { current_queue=nullptr;delete q;++queues_freed; }
    return nullptr;
}
extern "C" void tmr_init(tmr* t){t->handler=nullptr;t->arg=nullptr;t->wait_ms=0;}
extern "C" void tmr_start_dbg(tmr* t,std::uint64_t ms,tmr_h* h,void* a,
                              const char*,int) {
    if(std::this_thread::get_id()!=owner_thread)wrong_owner=true;
    t->handler=h;t->arg=a;t->wait_ms=ms;current_timer=t;
}
extern "C" void tmr_cancel(tmr* t){t->handler=nullptr;current_timer=nullptr;}
extern "C" void re_cancel(void) {
    if(std::this_thread::get_id()!=owner_thread)wrong_owner=true;
    cancelled=true;
    if(current_queue) current_queue->cv.notify_all();
}
extern "C" int re_main(re_signal_h*) {
    if(std::this_thread::get_id()!=owner_thread)wrong_owner=true;
    while(!cancelled) {
        auto* timer=current_timer;
        if(timer && timer->handler) {
            auto* cb=timer->handler;void* ctx=timer->arg;
            timer->handler=nullptr;
            cb(ctx);
        }
        if(cancelled)break;
        int command=0;
        {
            std::unique_lock<std::mutex> lock(current_queue->mutex);
            current_queue->cv.wait_for(lock,2ms,[&] {
                return !current_queue->commands.empty() || cancelled;
            });
            if(!current_queue->commands.empty()){
                command=current_queue->commands.front();
                current_queue->commands.pop_front();
            }
        }
        if(command) current_queue->handler(command,nullptr,current_queue->context);
    }
    return 0;
}
extern "C" int bevent_register(bevent_h* fn,void* ctx) {
    if(std::this_thread::get_id()!=owner_thread || event_handler) return 80;
    event_handler=fn;event_context=ctx;++callbacks_added;return 0;
}
extern "C" void bevent_unregister(bevent_h* fn) {
    if(std::this_thread::get_id()!=owner_thread || event_handler!=fn)
        wrong_owner=true;
    else {event_handler=nullptr;event_context=nullptr;++callbacks_removed;}
}
extern "C" ua* bevent_get_ua(const bevent* ev){return ev?ev->owner:nullptr;}
extern "C" call* bevent_get_call(const bevent* ev){
    return ev?ev->active_call:nullptr;
}
extern "C" bool ua_isregistered(const ua* account) {
    return account==&selected && sdk_registered;
}
extern "C" int ua_register(ua* account) {
    if(account!=&selected || std::this_thread::get_id()!=owner_thread)
        return 81;
    ++native_register;
    sdk_registered=false;
    bevent e{account,nullptr};
    if(event_handler) event_handler(BEVENT_REGISTERING,&e,event_context);
    return 0;
}
extern "C" void ua_unregister(ua* account) {
    if(account!=&selected || std::this_thread::get_id()!=owner_thread)
        wrong_owner=true;
    ++native_unregister;
    sdk_registered=false;
    bevent e{account,nullptr};
    if(event_handler) event_handler(BEVENT_UNREGISTERING,&e,event_context);
}

static bool await_event(axv::BaresipNativeRegistrationHost& host,
                        axv::RegistrationDispatchKind type) {
    const auto until=std::chrono::steady_clock::now()+2s;
    while(std::chrono::steady_clock::now()<until) {
        if(host.snapshot().last_dispatch==type) return true;
        std::this_thread::sleep_for(2ms);
    }
    return false;
}

int main() {
    using namespace axv;
    std::atomic<std::uint64_t> virtual_ms{1000};
    BaresipNativeRegistrationHost host(
        [&]() -> ua* {
            if(std::this_thread::get_id()!=owner_thread)wrong_owner=true;
            if(emit_ok.exchange(false) && event_handler){
                sdk_registered=true;
                bevent e{&selected,nullptr};
                event_handler(BEVENT_REGISTER_OK,&e,event_context);
            }
            if(emit_shutdown.exchange(false) && event_handler){
                sdk_registered=false;
                bevent e{&selected,nullptr};
                event_handler(BEVENT_SHUTDOWN,&e,event_context);
            }
            return &selected;
        },
        [&]{return virtual_ms.load();},5ms);

    // GUI-facing code never handles ua* or accesses Baresip C API.
    if(!host.start() || !host.snapshot().running) return 1;
    const auto first=host.generation();
    if(!first || host.start() || host.submit(999) ||
       host.submit_for_generation(first+1,2000)) return 2;
    std::thread gui([&]{if(!host.submit(2000))wrong_owner=true;});
    gui.join();
    if(!await_event(host,RegistrationDispatchKind::Submitted) ||
       native_register!=1 || !event_handler) return 3;
    emit_ok=true;
    if(!await_event(host,RegistrationDispatchKind::Confirmed)) return 4;
    if(!host.cancel() || !await_event(host,RegistrationDispatchKind::Cancelled) ||
       native_unregister!=1 || event_handler ||
       callbacks_added!=callbacks_removed) return 5;
    if(!host.stop() || host.snapshot().running || host.submit(2000) ||
       host.generation() || libre_started!=libre_closed) return 6;

    // Recreate entire real-loop control-plane with a monotonic generation.
    virtual_ms=5000;
    if(!host.start() || host.generation()<=first ||
       host.submit_for_generation(first,2000)) return 7;
    if(!host.submit(1000) ||
       !await_event(host,RegistrationDispatchKind::Submitted) ||
       native_register!=2) return 8;
    virtual_ms=6200;
    if(!await_event(host,RegistrationDispatchKind::DeadlineExpired) ||
       native_unregister!=2 || event_handler) return 9;
    if(!host.stop()) return 10;

    // A terminal SDK UA shutdown invalidates the epoch; no ua_unregister
    // may dereference the already-dead UA.
    virtual_ms=8000;
    if(!host.start() || !host.submit(2000) ||
       !await_event(host,RegistrationDispatchKind::Submitted)) return 11;
    emit_shutdown=true;
    if(!await_event(host,RegistrationDispatchKind::DeadlineExpired) ||
       native_unregister!=2 || event_handler) return 12;
    if(!host.stop()) return 13;

    // A failed UI wake must revoke the admission and prevent registration.
    virtual_ms=10000;
    if(!host.start()) return 14;
    const auto before=native_register.load();
    deny_push=true;
    if(host.submit(2000)) return 15;
    deny_push=false;
    if(!await_event(host,RegistrationDispatchKind::Cancelled) ||
       native_register!=before) return 16;
    if(!host.stop()) return 17;

    // No native callback/queue can leak across worker generations.
    deny_init=true;
    if(host.start() || host.generation() || host.snapshot().running ||
       host.snapshot().reactor_fault!=LibreReactorFault::InitRejected)
       return 18;
    deny_init=false;
    if(!host.start() || !host.stop()) return 19;

    if(wrong_owner || callbacks_added!=callbacks_removed ||
       libre_started!=6 || libre_closed!=5 || queues_freed!=5 ||
       native_register!=3 || native_unregister!=2) return 20;
    return 0;
}
