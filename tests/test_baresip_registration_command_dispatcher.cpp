#include "../src/baresip_registration_command_dispatcher.h"
#include <atomic>
#include <thread>

static bevent_h* handler=nullptr;
static void* handler_arg=nullptr;
static unsigned registers=0,unregisters=0,installed=0,removed=0;
static bool registered=false;
extern "C" int bevent_register(bevent_h* h,void* ctx) {
    if(handler) return 11;
    handler=h;handler_arg=ctx;++installed;return 0;
}
extern "C" void bevent_unregister(bevent_h* h) {
    if(handler==h){handler=nullptr;handler_arg=nullptr;++removed;}
}
extern "C" ua* bevent_get_ua(const bevent* e) {
    return e?e->owner:nullptr;
}
extern "C" call* bevent_get_call(const bevent* e) {
    return e?e->active_call:nullptr;
}
extern "C" bool ua_isregistered(const ua* p) {
    return p && p->test_id==1 && registered;
}
extern "C" int ua_register(ua* p) {
    if(!p) return 12;
    ++registers;
    bevent ev{p,nullptr};
    if(handler) handler(BEVENT_REGISTERING,&ev,handler_arg);
    return 0;
}
extern "C" void ua_unregister(ua* p) {
    if(!p) return;
    ++unregisters;
    registered=false;
    bevent ev{p,nullptr};
    if(handler) handler(BEVENT_UNREGISTERING,&ev,handler_arg);
}

int main() {
    using namespace axv;
    RegistrationCommandQueue queue;
    BaresipEventMailbox mailbox;
    BaresipCallbackLifecycle life(mailbox);
    BaresipSdkEventAdapter adapter(life);
    BaresipSdkRegistrationGateway gateway(adapter);
    BaresipRegistrationCommandDispatcher dispatcher(queue,gateway);
    ua selected{1};
    const auto generation=queue.begin_generation();
    if(!generation) return 1;

    // Real std::thread emulates UI producer; it cannot access native SDK.
    std::thread producer([&]{queue.submit(generation,3000);});
    producer.join();
    std::atomic<bool> foreign_blocked{false};
    std::thread alien([&] {
        foreign_blocked=dispatcher.tick(generation,0,&selected).kind==
            RegistrationDispatchKind::WrongOwnerThread;
    });
    alien.join();
    if(!foreign_blocked || registers || installed) return 2;

    auto result=dispatcher.tick(generation,100,&selected);
    if(result.kind!=RegistrationDispatchKind::Submitted ||
       registers!=1 || !adapter.active()) return 3;
    if(dispatcher.tick(generation,3099,&selected).kind!=
       RegistrationDispatchKind::Pending) return 4;
    if(dispatcher.tick(generation,3100,&selected).kind!=
       RegistrationDispatchKind::DeadlineExpired ||
       handler || unregisters!=1) return 5;
    if(queue.submit(generation,1000)) return 6;

    const auto next=queue.begin_generation();
    if(next<=generation || !queue.submit(next,5000)) return 7;
    result=dispatcher.tick(next,5000,&selected);
    if(result.kind!=RegistrationDispatchKind::Submitted || registers!=2)
        return 8;
    // A native register callback + UA status together confirm the new epoch.
    registered=true;
    bevent ev{&selected,nullptr};
    handler(BEVENT_REGISTER_OK,&ev,handler_arg);
    if(dispatcher.tick(next,6000,&selected).kind!=
       RegistrationDispatchKind::Confirmed) return 9;
    if(!queue.cancel(next)) return 10;
    if(dispatcher.tick(next,6001,&selected).kind!=
       RegistrationDispatchKind::Cancelled || handler ||
       unregisters!=2 || installed!=removed) return 11;

    const auto third=queue.begin_generation();
    if(third<=next || !queue.submit(third,5000)) return 12;
    // Cancellation must supersede even a queued submit.
    if(!queue.cancel(third)) return 13;
    if(dispatcher.tick(third,7000,&selected).kind!=
       RegistrationDispatchKind::Cancelled || registers!=2) return 14;

    // Buffer overflow results in stop, NEVER processing 16 partial SUBMITs.
    const auto fourth=queue.begin_generation();
    if(fourth<=third) return 15;
    for(std::size_t i=0;i<RegistrationCommandQueue::kCapacity;++i)
        if(!queue.submit(fourth,5000)) return 16;
    if(queue.submit(fourth,5000)) return 17;
    if(dispatcher.tick(fourth,9000,&selected).kind!=
       RegistrationDispatchKind::OverflowStopped || registers!=2)
        return 18;
    if(queue.begin_generation()<=fourth || installed!=removed)
        return 19;
    return 0;
}
