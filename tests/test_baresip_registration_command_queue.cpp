#include "../src/baresip_registration_command_queue.h"
#include <atomic>
#include <thread>

int main() {
    using namespace axv;
    RegistrationCommandQueue q;
    if(q.submit(1,5000) || q.cancel(1)) return 1;
    auto a=q.begin_generation();
    if(!a || q.begin_generation()) return 2;
    if(q.submit(a,0) || q.submit(a,999) || q.submit(a,120001)) return 3;
    std::atomic<int> ok{0};
    std::thread producer([&] {
        if(q.submit(a,5000)) ++ok;
        if(q.submit(a,6000)) ++ok;
    });
    producer.join();
    if(ok!=2) return 4;
    auto b=q.drain(a);
    if(b.size!=2 || b.overflow || b.generation!=a ||
       b.commands[0].kind!=RegistrationCommandKind::SubmitSelectedAccount ||
       b.commands[1].timeout_ms!=6000) return 5;
    if(q.drain(a).size) return 6;

    // Cancel deletes pending submissions and permanently closes generation.
    if(!q.submit(a,30000) || !q.cancel(a) || q.submit(a,1000) || q.cancel(a))
        return 7;
    if(q.begin_generation()) return 8;
    b=q.drain(a);
    if(b.size!=1 || b.commands[0].kind!=RegistrationCommandKind::Cancel)
        return 9;
    auto c=q.begin_generation();
    if(!c || c<=a || q.submit(a,10000) || q.cancel(a)) return 10;

    // Full queue + extra submit => overflow and discarded entire history.
    for(std::size_t i=0;i<RegistrationCommandQueue::kCapacity;++i)
        if(!q.submit(c,1000)) return 11;
    if(q.submit(c,1000) || q.submit(c,1000) || q.begin_generation())
        return 12;
    b=q.drain(c);
    if(!b.overflow || b.size || b.generation!=c) return 13;
    auto d=q.begin_generation();
    if(!d || d<=c || !q.submit(d,120000)) return 14;
    if(q.drain(c).generation || q.drain(d).size!=1) return 15;
    // A drained command is invalidated by a later UI cancellation.
    auto valid_generation=q.begin_generation(); // d remains open: must fail.
    if(valid_generation) return 17;
    if(!q.submit(d,1000)) return 18;
    auto drained=q.drain(d);
    if(drained.size!=1 || !q.still_admitted(drained)) return 19;
    if(!q.cancel(d) || q.still_admitted(drained)) return 20;
    auto cancellation=q.drain(d);
    if(cancellation.size!=1 ||
       cancellation.commands[0].kind!=RegistrationCommandKind::Cancel)
       return 21;
    q.invalidate();
    if(q.submit(d,1000) || q.cancel(d)) return 16;
    return 0;
}
