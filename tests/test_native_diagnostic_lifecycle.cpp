// Real SDK, synthetic modules: initialization rejects WASAPI before ua_alloc,
// so this regression probe never creates an account or sends SIP traffic.
#include "../src/baresip_sdk_headers.h"
#include "../src/baresip_sip_engine.h"
#include <chrono>
#include <cstdio>
#include <thread>

namespace {
int module_ok() { return 0; }
int module_reject() { return 95; }
int module_close() { return 0; }
bool wait_for_failure(axv::BaresipSipEngine& engine) {
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (engine.status() != axv::SipStatus::Failed &&
           std::chrono::steady_clock::now() < until)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    return engine.status() == axv::SipStatus::Failed &&
           engine.diagnostic().find(L"wasapi: 95") != std::wstring::npos;
}
}
extern "C" {
extern const struct mod_export exports_g711 = {"synthetic_g711", "codec", module_ok, module_close};
extern const struct mod_export exports_wasapi = {"synthetic_wasapi", "audio", module_reject, module_close};
}
namespace axv {
AudioInventory inspect_windows_audio_devices() { return {}; }
}

int main() {
    axv::BaresipSipEngine engine;
    const auto idle_diagnostic = engine.diagnostic();
    const axv::SipAccount account{L"127.0.0.1", L"1001", 5060, false};
    if (!engine.configure_authenticated(account, L"", "synthetic-only", 1000) ||
        !engine.register_account() || !wait_for_failure(engine)) {
        std::fputs("fixture did not produce the expected SDK failure\n", stderr);
        return 2;
    }
    engine.stop();
    if (engine.status() != axv::SipStatus::Disconnected ||
        engine.observation().process != axv::RuntimeProcess::Stopped) return 3;
    bool failed = false;
    if (engine.diagnostic() != idle_diagnostic) {
        std::fputs("stop retains the previous SDK failure diagnostic\n", stderr);
        failed = true;
    }
    engine.stop();
    if (engine.diagnostic() != idle_diagnostic) {
        std::fputs("repeated stop retains the previous SDK failure diagnostic\n", stderr);
        failed = true;
    }
    if (!engine.configure_authenticated(account, L"", "fresh-synthetic-only", 1000)) return 4;
    if (engine.diagnostic() != idle_diagnostic) {
        std::fputs("new configuration retains the previous SDK failure diagnostic\n", stderr);
        failed = true;
    }
    // Retry is still allowed and its new SDK failure must remain visible.
    if (!engine.register_account() || !wait_for_failure(engine)) return 5;
    engine.stop();
    return failed ? 1 : 0;
}
