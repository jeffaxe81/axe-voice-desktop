#include "../src/baresip_registration_lab_worker.h"
#include <atomic>
#include <chrono>
#include <thread>
using namespace std::chrono_literals;
static bevent_h* handler=nullptr;
static void* context=nullptr;
static std::atomic<unsigned> starts{0},stops{0},added{0},removed{0};
static std::atomic<bool> owner_ok{true},confirm{false},shutdown_event{false};
static std::thread::id sdk_thread;
static bool registered=false;
extern "C" int bevent_register(bevent_h* h,void* arg) {
    if(handler || std::this_thread::get_id()!=sdk_thread) return 17;
    handler=h;context=arg;++added;return 0;
}
extern "C" void bevent_unregister(bevent_h* h) {
    if(handler==h && std::this_thread::get_id()==sdk_thread){
        handler=nullptr;context=nullptr;++removed;
    } else owner_ok=false;
}
extern "C" ua* bevent_get_ua(const bevent* e){return e?e->owner:nullptr;}
extern "C" call* bevent_get_call(const bevent* e){return e?e->active_call:nullptr;}
extern "C" bool ua_isregistered(const ua* u){
    return u && u->test_id==1 && registered;
}
extern "C" int ua_register(ua* u){
    if(!u || std::this_thread::get_id()!=sdk_thread) return 18;
    ++starts;registered=false;
    bevent e{u,nullptr};if(handler) handler(BEVENT_REGISTERING,&e,context);
    return 0;
}
extern "C" void ua_unregister(ua* u){
    if(!u || std::this_thread::get_id()!=sdk_thread){owner_ok=false;return;}
    ++stops;registered=false;
    bevent e{u,nullptr};if(handler) handler(BEVENT_UNREGISTERING,&e,context);
}
int main(){
    using namespace axv;
    ua account{1};
    std::atomic<std::uint64_t> time_ms{1000};
    BaresipRegistrationLabWorker worker(
      [&]()->ua*{
        if(std::this_thread::get_id()!=sdk_thread) owner_ok=false;
        return &account;
      },
      [&]{
        if(sdk_thread==std::thread::id{}) sdk_thread=std::this_thread::get_id();
        if(std::this_thread::get_id()!=sdk_thread) owner_ok=false;
        if(confirm.exchange(false) && handler){
            registered=true;bevent e{&account,nullptr};
            handler(BEVENT_REGISTER_OK,&e,context);
        }
        if(shutdown_event.exchange(false) && handler){
            registered=false;bevent e{&account,nullptr};
            handler(BEVENT_SHUTDOWN,&e,context);
        }
      },[&]{return time_ms.load();},5ms);
    if(!worker.start() || !worker.snapshot().running || worker.start()) return 1;
    const auto generation=worker.snapshot().generation;
    if(!generation || !worker.submit(3000) ||
       !worker.await_event(RegistrationDispatchKind::Submitted,2s) ||
       starts!=1 || added!=1) return 2;
    confirm=true;
    if(!worker.await_event(RegistrationDispatchKind::Confirmed,2s)) return 3;
    if(!worker.cancel() ||
       !worker.await_event(RegistrationDispatchKind::Cancelled,2s) ||
       stops!=1 || added!=removed) return 4;
    if(!worker.stop() || worker.snapshot().running || worker.submit(3000)) return 5;

    sdk_thread={};time_ms=5000;
    if(!worker.start() || worker.snapshot().generation<=generation) return 6;
    if(!worker.submit(1000) ||
       !worker.await_event(RegistrationDispatchKind::Submitted,2s)) return 7;
    time_ms=7000;
    if(!worker.await_event(RegistrationDispatchKind::DeadlineExpired,2s) ||
       stops!=2 || added!=removed) return 8;
    if(!worker.stop()) return 9;

    sdk_thread={};time_ms=10000;
    if(!worker.start() || !worker.submit(3000) ||
       !worker.await_event(RegistrationDispatchKind::Submitted,2s)) return 10;
    shutdown_event=true;
    if(!worker.await_event(RegistrationDispatchKind::DeadlineExpired,2s) ||
       stops!=2) return 11;
    if(!worker.stop() || worker.snapshot().running ||
       !owner_ok || added!=removed || starts!=3) return 12;
    return 0;
}
