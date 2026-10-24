#include "../src/baresip_native_libre_reactor.h"
#include <atomic>
#include <chrono>
#include <thread>
// This program requires the separately audited real libre SDK.
// No Baresip core initialization, UA, SIP sockets or account operations.
int main() {
    std::atomic<unsigned> callbacks{0};
    axv::BaresipNativeLibreReactor loop([&]{++callbacks;});
    if (!loop.start()) return 1;
    if (!loop.wake()) { (void)loop.stop(); return 2; }
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    if (!loop.stop()) return 3;
    const auto state=loop.snapshot();
    return !state.running && state.ticks>0 &&
           callbacks>0 && state.fault==axv::LibreReactorFault::None ? 0 : 4;
}
