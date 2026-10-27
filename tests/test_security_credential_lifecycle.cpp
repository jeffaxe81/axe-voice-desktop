// Actual SDK fault-injection lifecycle probe. Compile only the reactor source
// with libre_init=axv_fixture_libre_init and
// mqueue_alloc=axv_fixture_mqueue_alloc; SDK libraries remain unchanged.
// Modes: 1 init rejection; 2 queue rejection; 3 stop; 4 replace credential.
// Run each mode in a separate process; checks remain active under NDEBUG.
// Reject reactor initialization before any account creation/registration;
// no SIP request is sent. Observe only live C++
// allocations large enough to contain this test's unique synthetic password.
#include "../src/baresip_sdk_headers.h"
#include "../src/baresip_sip_engine.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>

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

int fault = 0;
extern "C" int axv_fixture_libre_init() { return fault == 1 ? 95 : libre_init(); }
extern "C" int axv_fixture_mqueue_alloc(struct mqueue** out, mqueue_h* handler, void* arg) {
    return fault == 2 ? 95 : mqueue_alloc(out, handler, arg);
}
int main(int argc, char** argv) {
    fault = argc > 1 ? std::atoi(argv[1]) : 1;
    if (fault < 1 || fault > 4) return 10;
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
    if (fault == 3) {
        engine.stop();
        if (copies() != 0 || engine.register_account() ||
            engine.status() != axv::SipStatus::Disconnected) {
            std::fputs("stop before startup retained credential or reusable configuration\n", stderr);
            return 6;
        }
        engine.stop(); // Repeated stop must remain safe.
        return 0;
    }
    if (fault == 4) {
        std::string replacement(synthetic);
        replacement[0] = replacement[0] == 'X' ? 'Y' : 'X';
        if (!engine.configure_authenticated({L"127.0.0.1", L"1002", 5060, false},
                                            L"", std::move(replacement), 1000)) return 7;
        if (copies() != 0) {
            std::fputs("reconfiguration retains superseded pending synthetic password\n", stderr);
            return 8;
        }
        engine.stop();
        return 0;
    }
    if (engine.register_account()) {
        std::fputs("injected reactor startup rejection was not returned\n", stderr);
        engine.stop();
        return 4;
    }
    const auto retained = copies();
    const auto status = engine.status();
    const auto security = engine.security_status();
    if(!security.policy_selected || security.tls_required || security.running_process || security.verified_registration)return 11;
    engine.stop();
    if (copies() != 0 || engine.register_account()) {
        std::fputs("stop after startup failure retained credential or reusable configuration\n", stderr);
        return 9;
    }
    if (retained) {
        std::fputs("rejected reactor startup retains pending synthetic password until explicit stop\n", stderr);
        return 1;
    }
    if (status != axv::SipStatus::Failed) {
        std::fputs("rejected reactor startup does not publish Failed status\n", stderr);
        return 5;
    }
    return 0;
}
