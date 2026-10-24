#include "../src/baresip_native_thread_guard.h"
#include <atomic>
#include <thread>
int main() {
    axv::BaresipNativeThreadGuard gate;
    if (!gate.is_owner()) return 1;
    std::atomic<bool> admitted{true};
    std::thread wrong([&] { admitted.store(gate.is_owner()); });
    wrong.join();
    if (admitted || !gate.is_owner()) return 2;
    // The guard is tied to its original owner, never rebound by another thread.
    std::thread another([&] { admitted.store(gate.is_owner()); });
    another.join();
    if (admitted || !gate.is_owner()) return 3;
    return 0;
}
