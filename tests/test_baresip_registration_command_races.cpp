#include "../src/baresip_registration_command_dispatcher.h"
#include <atomic>
#include <thread>
#include <cstdint>

static bevent_h* native_handler = nullptr;
static void* native_context = nullptr;
static unsigned registrations = 0, unregistrations = 0;
static unsigned added = 0, removed = 0;
static bool ua_registered = false;
static axv::RegistrationCommandQueue* request_queue = nullptr;
static std::uint64_t current_generation = 0;
static int injection = 0;
static std::atomic<bool> injected{false};

extern "C" int bevent_register(bevent_h* h,void* c) {
    if(native_handler) return 51;
    native_handler=h; native_context=c; ++added; return 0;
}
extern "C" void bevent_unregister(bevent_h* h) {
    if(native_handler==h) {
        native_handler=nullptr; native_context=nullptr; ++removed;
    }
}
extern "C" ua* bevent_get_ua(const bevent* e) {
    return e?e->owner:nullptr;
}
extern "C" call* bevent_get_call(const bevent* e) {
    return e?e->active_call:nullptr;
}
extern "C" bool ua_isregistered(const ua* u) {
    return u && u->test_id==1 && ua_registered;
}
extern "C" int ua_register(ua* u) {
    if(!u) return 52;
    ++registrations;
    bevent starting{u,nullptr};
    if(native_handler) native_handler(BEVENT_REGISTERING,&starting,native_context);
    // Simulates cross-thread Cancel or queue overflow WHILE the native
    // ua_register call is blocked on the event-loop owner thread.
    if(injection==1) {
        std::thread ui([] {
            injected=request_queue->cancel(current_generation);
        });
        ui.join();
    } else if(injection==2) {
        std::thread ui([] {
            bool full=true;
            for(std::size_t n=0;n<axv::RegistrationCommandQueue::kCapacity;++n)
                full &= request_queue->submit(current_generation,2000);
            injected=full && !request_queue->submit(current_generation,2000);
        });
        ui.join();
    }
    return 0;
}
extern "C" void ua_unregister(ua* u) {
    if(!u) return;
    ++unregistrations;
    ua_registered=false;
    bevent ev{u,nullptr};
    if(native_handler) native_handler(BEVENT_UNREGISTERING,&ev,native_context);
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
    request_queue=&queue;

    // 1: UI cancellation overtakes an already drained Submit.
    current_generation=queue.begin_generation();
    if(!current_generation || !queue.submit(current_generation,3000)) return 1;
    injection=1; injected=false;
    auto out=dispatcher.tick(current_generation,100,&selected);
    if(!injected || out.kind!=RegistrationDispatchKind::SupersededDuringDispatch ||
       native_handler || gateway.submitted() || mailbox.drain().session ||
       registrations!=1 || unregistrations!=1 || added!=removed) return 2;
    if(queue.submit(current_generation,2000) || queue.cancel(current_generation))
        return 3;

    // 2: a producer fills the queue during the SDK call and overflows it.
    current_generation=queue.begin_generation();
    if(!current_generation || !queue.submit(current_generation,3000)) return 4;
    injection=2; injected=false;
    out=dispatcher.tick(current_generation,1000,&selected);
    if(!injected || out.kind!=RegistrationDispatchKind::SupersededDuringDispatch ||
       native_handler || gateway.submitted() || mailbox.drain().session ||
       registrations!=2 || unregistrations!=2 || added!=removed) return 5;
    if(queue.submit(current_generation,2000)) return 6;

    // 3: without concurrent invalidation a submission remains pending until
    // a real callback confirms it. Normal Cancel still closes the session.
    current_generation=queue.begin_generation();
    if(!current_generation || !queue.submit(current_generation,3000)) return 7;
    injection=0;
    out=dispatcher.tick(current_generation,5000,&selected);
    if(out.kind!=RegistrationDispatchKind::Submitted ||
       registrations!=3 || !gateway.submitted()) return 8;
    ua_registered=true;
    bevent registered_event{&selected,nullptr};
    native_handler(BEVENT_REGISTER_OK,&registered_event,native_context);
    if(dispatcher.tick(current_generation,5001,&selected).kind!=
       RegistrationDispatchKind::Confirmed) return 9;
    if(!queue.cancel(current_generation) ||
       dispatcher.tick(current_generation,5002,&selected).kind!=
          RegistrationDispatchKind::Cancelled ||
       unregistrations!=3 || added!=removed || native_handler) return 10;
    return 0;
}
