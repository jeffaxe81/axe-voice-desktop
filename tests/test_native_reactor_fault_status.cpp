// Standalone, always-on fault diagnostics regression. No UA, sockets, SDK or
// credentials: the C boundary below supplies deterministic loop failures.
#include "../src/baresip_native_libre_reactor.h"
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>
using namespace std::chrono_literals;

struct mqueue {};
namespace {
thread_local mqueue queue;
thread_local tmr* timer=nullptr;
thread_local bool cancelled=false;
int main_error=0; // Set only while no reactor thread is alive.
int failures=0;
void check(bool condition,const char* message) {
    if (!condition) {std::fprintf(stderr,"FAIL: %s\n",message);++failures;}
}
}
extern "C" int libre_init() {cancelled=false;return 0;}
extern "C" void libre_close() {}
extern "C" int mqueue_alloc(mqueue** out,mqueue_h*,void*) {*out=&queue;return 0;}
// Timer invokes the tick synchronously, so no queue synchronization is needed.
extern "C" int mqueue_push(mqueue*,int,void*) {return 0;}
extern "C" void* mem_deref(void*) {return nullptr;}
extern "C" void tmr_init(tmr* value) {*value={};}
extern "C" void tmr_start_dbg(tmr* value,std::uint64_t delay,tmr_h* hook,
                                void* arg,const char*,int) {
    value->handler=hook;value->arg=arg;value->wait_ms=delay;timer=value;
}
extern "C" void tmr_cancel(tmr*) {timer=nullptr;}
extern "C" void re_cancel() {cancelled=true;}
extern "C" int re_main(re_signal_h*) {
    // One bounded owner callback, then a configured native loop exit.
    if (!cancelled && timer && timer->handler) timer->handler(timer->arg);
    return main_error;
}
bool await_exit(axv::BaresipNativeLibreReactor& loop) {
    const auto deadline=std::chrono::steady_clock::now()+1s;
    while (loop.snapshot().running && std::chrono::steady_clock::now()<deadline)
        std::this_thread::sleep_for(1ms);
    return !loop.snapshot().running;
}
int main() {
    using axv::BaresipNativeLibreReactor;
    using axv::LibreReactorFault;
    {
        unsigned stops=0;
        BaresipNativeLibreReactor loop([]{},[]{throw std::runtime_error("start");},
                                      [&]{++stops;},1ms);
        check(!loop.start(),"throwing startup rejects start");
        check(loop.stop(),"startup failure joins");
        auto s=loop.snapshot();
        check(!s.running && s.generation==0 && s.ticks==0 &&
              s.fault==LibreReactorFault::HookFailed,"startup failure snapshot survives join");
        check(stops==1,"failed startup cleans partial state once without a completed generation");
    }
    {
        main_error=0;
        BaresipNativeLibreReactor loop([]{throw std::runtime_error("tick");});
        (void)loop.start(); // Loop can finish before start's waiter reacquires mutex.
        check(await_exit(loop),"tick exits autonomously");
        check(loop.stop(),"tick failure joins");
        auto s=loop.snapshot();
        check(!s.running && s.generation==1 && s.ticks==0 &&
              s.fault==LibreReactorFault::HookFailed,"failed tick is not counted as completed");
        check(!loop.wake(),"failed loop rejects wake");
        check(loop.stop() && loop.snapshot().fault==LibreReactorFault::HookFailed,
              "idempotent stop preserves fault");
    }
    {
        main_error=37;
        BaresipNativeLibreReactor loop([]{}, {},[]{throw std::runtime_error("cleanup");});
        (void)loop.start();
        check(await_exit(loop),"main failure exits autonomously");
        check(loop.stop(),"main-loop and cleanup failure join");
        auto s=loop.snapshot();
        check(!s.running && s.generation==1 && s.ticks<=1,"failed loop stops with bounded ticks");
        check(s.fault==LibreReactorFault::MainLoopFailed,
              "cleanup exception must preserve original main-loop execution failure");
    }
    {
        main_error=37;
        BaresipNativeLibreReactor loop([]{throw std::runtime_error("tick");});
        (void)loop.start();
        check(await_exit(loop),"combined failure exits autonomously");
        check(loop.stop(),"tick and main-loop failure join");
        check(loop.snapshot().fault==LibreReactorFault::HookFailed,
              "main-loop error must preserve original tick execution failure");
    }
    {
        main_error=0;
        bool reject_tick=true; // Mutated only after the prior worker is joined.
        BaresipNativeLibreReactor loop([&]{if(reject_tick) throw std::runtime_error("tick");});
        (void)loop.start();
        check(await_exit(loop) && loop.stop(),"failed generation joins before restart");
        check(loop.snapshot().generation==1 && loop.snapshot().ticks==0,
              "failed first generation counters");
        reject_tick=false;
        (void)loop.start();
        check(await_exit(loop) && loop.stop(),"restarted generation joins");
        auto s=loop.snapshot();
        check(!s.running && s.generation==2 && s.ticks==1 &&
              s.fault==LibreReactorFault::None,
              "restart resets current-run fault and ticks, retaining generation count");
    }
    return failures?1:0;
}
