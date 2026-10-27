// Native SDK integration probe. Deliberately reject an audio module before
// account creation/registration; no SIP request is sent. Observe only live C++
// allocations large enough to contain this test's unique synthetic password.
#include "../src/baresip_sdk_headers.h"
#include "../src/baresip_sip_engine.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <thread>

namespace {
constexpr std::size_t secret_size = 997;
char synthetic[secret_size + 1];
void* candidates[64]{};
std::atomic_flag guard = ATOMIC_FLAG_INIT;
bool overflow = false;
bool capture_allocations = true;
struct Lock {
    Lock() { while (guard.test_and_set(std::memory_order_acquire)) {} }
    ~Lock() { guard.clear(std::memory_order_release); }
};
std::size_t copies() {
    Lock lock;
    std::size_t count = 0;
    for (const auto p : candidates)
        if (p && std::memcmp(p, synthetic, sizeof(synthetic)) == 0) ++count;
    return count;
}
int module_ok() { return 0; }
int module_reject() { return 95; }
int module_close() { return 0; }
}

void* operator new(std::size_t size) {
    void* p = std::malloc(size ? size : 1);
    if (!p) throw std::bad_alloc();
    if (size >= sizeof(synthetic)) {
        Lock lock;
        if (capture_allocations) {
            bool inserted = false;
            for (auto& slot : candidates) if (!slot) { slot = p; inserted = true; break; }
            if (!inserted) overflow = true;
        }
    }
    return p;
}
void operator delete(void* p) noexcept {
    { Lock lock; for (auto& slot : candidates) if (slot == p) slot = nullptr; }
    std::free(p);
}
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }

extern "C" {
extern const struct mod_export exports_g711 = {"synthetic_g711", "codec", module_ok, module_close};
extern const struct mod_export exports_wasapi = {"synthetic_wasapi", "audio", module_reject, module_close};
}
namespace axv {
AudioInventory inspect_windows_audio_devices() { return {}; }
}

int main() {
    for (std::size_t i = 0; i < secret_size; ++i)
        synthetic[i] = static_cast<char>('!' + ((i * 31 + 7) % 90));
    synthetic[secret_size] = '\0';
    axv::BaresipSipEngine engine;
    if (!engine.configure_authenticated({L"127.0.0.1", L"1001", 5060, false},
                                        L"", std::string(synthetic), 1000)) return 2;
    { Lock lock; capture_allocations = false; }
    if (overflow || copies() != 1) {
        std::fputs("probe cannot uniquely identify pending synthetic password\n", stderr);
        return 3;
    }
    if (!engine.register_account()) return 4;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (engine.status() != axv::SipStatus::Failed && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (engine.status() != axv::SipStatus::Failed ||
        engine.diagnostic().find(L"wasapi") == std::wstring::npos) {
        std::fputs("expected synthetic module rejection was not observed\n", stderr);
        return 5;
    }
    // Failure status is published after the fail handler's cleanup. Do not stop
    // first: that would hide retention until an explicit user stop/destruction.
    const auto retained = copies();
    engine.stop();
    if (retained) {
        std::fputs("failed initialization retains pending synthetic password until explicit stop\n", stderr);
        return 1;
    }
    return 0;
}
