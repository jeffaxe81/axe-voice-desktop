#include "../src/baresip_native_registration_host.h"
#include "../src/baresip_native_ua_factory.h"
#include "../src/baresip_native_core_bootstrap.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <stdexcept>
#include <string>
#include <utility>
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
static std::atomic<bool> confirm_on_register{false};
static std::atomic<bool> deny_ua_alloc{false}, partial_ua_alloc{false};
static std::atomic<bool> enforce_core_gate{false}, fake_core_prepared{false};
static std::atomic<unsigned> allocated_ua{0}, freed_ua{0};
static std::string last_account_aor; // Test-only: never enters application logs.

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
extern "C" int ua_alloc(ua** out, const char* aor) {
    if (std::this_thread::get_id()!=owner_thread || !out || !aor) {
        wrong_owner=true;
        return 70;
    }
    if (enforce_core_gate && !fake_core_prepared) wrong_owner=true;
    if (deny_ua_alloc) { *out=nullptr; return 71; }
    last_account_aor=aor;
    *out=&selected;
    ++allocated_ua;
    return partial_ua_alloc ? 72 : 0;
}
extern "C" unsigned ua_destroy(ua* account) {
    if (std::this_thread::get_id()!=owner_thread ||
        account!=&selected || event_handler || sdk_registered)
        wrong_owner=true;
    ++freed_ua;
    return 0;
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
    if(confirm_on_register) {
        sdk_registered=true;
        if(event_handler) event_handler(BEVENT_REGISTER_OK,&e,event_context);
    }
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

// Dedicated CTest mode: real C++ owner/GUI threads and fake native SDK.
static int test_owned_ua_lifecycle() {
    using namespace axv;
    using Hooks = BaresipNativeRegistrationHost::NativeUaLifecycleHooks;
    std::atomic<unsigned> created{0}, destroyed{0};
    std::atomic<bool> deny{false}, throw_create{false};

    Hooks hooks;
    hooks.create = [&]() -> ua* {
        if (std::this_thread::get_id()!=owner_thread) wrong_owner=true;
        if (throw_create) throw std::runtime_error("fake provider failure");
        if (deny) return nullptr;
        ++created;
        return &selected;
    };
    hooks.destroy = [&](ua* ptr) {
        if (std::this_thread::get_id()!=owner_thread ||
            ptr!=&selected || event_handler != nullptr) wrong_owner=true;
        ++destroyed;
    };
    BaresipNativeRegistrationHost owned(std::move(hooks), {}, 5ms);
    if (owned.snapshot().account_provisioned || !owned.start() ||
        !owned.snapshot().running || !owned.snapshot().account_provisioned ||
        created!=1 || destroyed!=0) return 101;

    const auto generation=owned.generation();
    confirm_on_register=true;
    std::thread gui([&] {
        if (!owned.submit_for_generation(generation,2000)) wrong_owner=true;
    });
    gui.join();
    if (!await_event(owned, RegistrationDispatchKind::Confirmed) ||
        native_register!=1 || !owned.snapshot().account_provisioned)
        return 102;
    if (!owned.stop() || owned.snapshot().running ||
        owned.snapshot().account_provisioned || owned.generation()!=0 ||
        created!=1 || destroyed!=1 || event_handler) return 103;
    confirm_on_register=false;

    deny=true;
    if (owned.start() || owned.snapshot().running ||
        owned.snapshot().account_provisioned ||
        owned.snapshot().reactor_fault!=LibreReactorFault::HookFailed ||
        created!=1 || destroyed!=1) return 104;
    deny=false;

    throw_create=true;
    if (owned.start() || owned.snapshot().reactor_fault!=LibreReactorFault::HookFailed ||
        owned.snapshot().account_provisioned ||
        created!=1 || destroyed!=1) return 105;
    throw_create=false;

    if (!owned.start() || !owned.snapshot().account_provisioned ||
        created!=2 || owned.generation()<=generation ||
        owned.submit_for_generation(generation, 2000)) return 106;
    if (!owned.stop() || owned.snapshot().account_provisioned ||
        destroyed!=2 || wrong_owner ||
        callbacks_added!=callbacks_removed ||
        native_register!=1 || native_unregister!=1) return 107;

    // Missing destroy is a configuration failure, not permission to leak UA.
    Hooks invalid;
    invalid.create=[&]() -> ua* { ++created; return &selected; };
    BaresipNativeRegistrationHost broken(std::move(invalid),{},5ms);
    if (broken.start() || broken.snapshot().running ||
        broken.snapshot().account_provisioned ||
        broken.snapshot().reactor_fault!=LibreReactorFault::HookFailed ||
        created!=2 || destroyed!=2) return 108;
    if (libre_started!=libre_closed || queues_freed!=libre_closed ||
        callbacks_added!=callbacks_removed) return 109;
    return 0;
}

// SDK UA factory test. The SDK below is a fake: no socket, password,
// TLS negotiation, RTP, or real account authentication occurs.
static int test_sdk_native_factory() {
    using namespace axv;
    const NativeUaIdentity good{"1001","pbx.example.com",5061,true};
    const auto aor=native_ua_safe_aor(good);
    if (!aor || *aor!="<sip:1001@pbx.example.com:5061;transport=tls>")
        return 201;
    if (native_ua_safe_aor({"user:secret","pbx.example.com",5061,true}) ||
        native_ua_safe_aor({"1001","pbx.example.com;evil",5061,true}) ||
        native_ua_safe_aor({"1001","pbx.example.com",0,true}) ||
        native_ua_safe_aor({"1001","pbx.example.com",5060,false}) ||
        native_ua_safe_aor({"1001","-pbx.example.com",5061,true}) ||
        native_ua_safe_aor({"1001","pbx..example.com",5061,true}) ||
        native_ua_safe_aor({"1001@other","pbx.example.com",5061,true}) ||
        native_ua_safe_aor({"1001","pbx.example.com\nctrl_tcp",5061,true}) ||
        native_ua_safe_aor({"1001",std::string(64,'a')+".example",5061,true}))
        return 202;

    BaresipNativeRegistrationHost host(
        BaresipNativeUaFactory::make_owner_hooks(good), {}, 5ms);
    if (!host.start() || !host.snapshot().account_provisioned ||
        allocated_ua!=1 || freed_ua || last_account_aor!=*aor)
        return 203;
    const auto generation=host.generation();
    confirm_on_register=true;
    std::thread ui([&] {
        if (!host.submit_for_generation(generation, 2000)) wrong_owner=true;
    });
    ui.join();
    if (!await_event(host,RegistrationDispatchKind::Confirmed) ||
        native_register!=1 || !host.snapshot().account_provisioned)
        return 204;
    if (!host.stop() || host.snapshot().account_provisioned ||
        host.snapshot().running || host.generation() ||
        allocated_ua!=1 || freed_ua!=1 || native_unregister!=1)
        return 205;
    confirm_on_register=false;
    deny_ua_alloc=true;
    if (host.start() || host.snapshot().account_provisioned ||
        host.snapshot().reactor_fault!=LibreReactorFault::HookFailed ||
        allocated_ua!=1 || freed_ua!=1 || host.submit(2000))
        return 206;
    deny_ua_alloc=false;
    partial_ua_alloc=true;
    if (host.start() || host.snapshot().account_provisioned ||
        host.snapshot().reactor_fault!=LibreReactorFault::HookFailed ||
        allocated_ua!=2 || freed_ua!=2) return 207;
    partial_ua_alloc=false;
    if (!host.start() || host.generation()<=generation ||
        host.submit_for_generation(generation,2000) ||
        allocated_ua!=3 || !host.snapshot().account_provisioned)
        return 208;
    if (!host.stop() || freed_ua!=3) return 209;

    BaresipNativeRegistrationHost invalid(
        BaresipNativeUaFactory::make_owner_hooks(
            {"malicious@domain","pbx.example.com",5061,true}),{},5ms);
    if (invalid.start() || invalid.snapshot().account_provisioned ||
        invalid.snapshot().reactor_fault!=LibreReactorFault::HookFailed ||
        allocated_ua!=3 || freed_ua!=3) return 210;
    if (wrong_owner || callbacks_added!=callbacks_removed ||
        libre_started!=libre_closed || queues_freed!=libre_closed)
        return 211;
    return 0;
}

// Core readiness is an injected lab contract, NOT real baresip_init/ua_init.
// Test startup rollback and teardown order using the same owner thread.
static int test_native_core_bootstrap_gate() {
    using namespace axv;
    const NativeUaIdentity identity{"1002","pbx.example.com",5061,true};
    std::atomic<unsigned> prepare_count{0}, release_count{0};
    std::atomic<bool> deny_prepare{false}, throw_prepare{false};
    enforce_core_gate=true;
    BaresipNativeCoreHooks core;
    core.prepare=[&]()->bool {
        if (std::this_thread::get_id()!=owner_thread || fake_core_prepared)
            wrong_owner=true;
        ++prepare_count;
        if (throw_prepare) throw std::runtime_error("fake core init failed");
        if (deny_prepare) return false;
        fake_core_prepared=true;
        return true;
    };
    core.release=[&] {
        if (std::this_thread::get_id()!=owner_thread ||
            event_handler || sdk_registered || allocated_ua!=freed_ua)
            wrong_owner=true;
        fake_core_prepared=false;
        ++release_count;
    };
    BaresipNativeRegistrationHost host(
        make_core_gated_native_ua_hooks(identity, core), {}, 5ms);
    if (!host.start() || !host.snapshot().account_provisioned ||
        !fake_core_prepared || allocated_ua!=1 || prepare_count!=1 ||
        release_count!=0) return 301;
    const auto initial_generation=host.generation();
    confirm_on_register=true;
    if (!host.submit_for_generation(initial_generation,2000) ||
        !await_event(host,RegistrationDispatchKind::Confirmed) ||
        native_register!=1) return 302;
    if (!host.stop() || host.snapshot().account_provisioned ||
        fake_core_prepared || prepare_count!=1 || release_count!=1 ||
        allocated_ua!=1 || freed_ua!=1 || native_unregister!=1)
        return 303;
    confirm_on_register=false;

    deny_prepare=true;
    if (host.start() || host.snapshot().running ||
        host.snapshot().account_provisioned ||
        host.snapshot().reactor_fault!=LibreReactorFault::HookFailed ||
        allocated_ua!=1 || prepare_count!=2 || release_count!=2 ||
        fake_core_prepared) return 304;
    deny_prepare=false;

    throw_prepare=true;
    if (host.start() || host.snapshot().running ||
        host.snapshot().reactor_fault!=LibreReactorFault::HookFailed ||
        prepare_count!=3 || release_count!=3 || fake_core_prepared)
        return 305;
    throw_prepare=false;

    deny_ua_alloc=true;
    if (host.start() || host.snapshot().account_provisioned ||
        host.snapshot().reactor_fault!=LibreReactorFault::HookFailed ||
        prepare_count!=4 || release_count!=4 ||
        allocated_ua!=1 || fake_core_prepared) return 306;
    deny_ua_alloc=false;

    if (!host.start() || host.generation()<=initial_generation ||
        host.submit_for_generation(initial_generation,2000) ||
        allocated_ua!=2 || prepare_count!=5 || release_count!=4 ||
        !fake_core_prepared) return 307;
    if (!host.stop() || fake_core_prepared || prepare_count!=5 ||
        release_count!=5 || allocated_ua!=freed_ua) return 308;

    BaresipNativeCoreHooks incomplete;
    incomplete.prepare=[&]()->bool {
        ++prepare_count;
        return true;
    };
    BaresipNativeRegistrationHost blocked(
        make_core_gated_native_ua_hooks(identity, incomplete),{},5ms);
    if (blocked.start() || blocked.snapshot().account_provisioned ||
        blocked.snapshot().reactor_fault!=LibreReactorFault::HookFailed ||
        prepare_count!=5 || release_count!=5) return 309;

    if (wrong_owner || fake_core_prepared ||
        callbacks_added!=callbacks_removed ||
        libre_started!=libre_closed || queues_freed!=libre_closed)
        return 310;
    return 0;
}

int main(int argc, char** argv) {
    if (argc==2 && std::string(argv[1])=="--core-bootstrap")
        return test_native_core_bootstrap_gate();
    if (argc==2 && std::string(argv[1])=="--sdk-ua")
        return test_sdk_native_factory();
    if (argc==2 && std::string(argv[1])=="--owned-ua")
        return test_owned_ua_lifecycle();
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
