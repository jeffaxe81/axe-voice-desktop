#include "../src/baresip_sdk_headers.h"
#include "../src/baresip_sdk_event_adapter.h"
#include <type_traits>
#include <unordered_set>
#if defined(debug) || defined(info) || defined(warning) || defined(loglv) || defined(log)
#error Baresip logging macros leaked into the C++ ABI probe
#endif

// Link/ABI probe: CI uses only a fake SDK. The optional real-SDK executable
// is BUILT but NEVER RUN by the default build or by CTest.
static_assert(std::is_same_v<decltype(&bevent_register),
                             int (*)(bevent_h*, void*)>,
              "Incompatible bevent_register signature");
static_assert(std::is_same_v<decltype(&bevent_unregister),
                             void (*)(bevent_h*)>,
              "Incompatible bevent_unregister signature");

extern "C" void axv_sdk_probe_callback(enum bevent_ev, struct bevent*, void*) {}

// Volatile function pointers require real linker relocations. Merely compiling
// a static library may not resolve Baresip symbols, so this is an executable.
static auto volatile probe_register = &bevent_register;
static auto volatile probe_unregister = &bevent_unregister;
static auto volatile probe_get_ua = &bevent_get_ua;
static auto volatile probe_get_call = &bevent_get_call;
static auto volatile probe_connect = &ua_connect;
static auto volatile probe_answer = &ua_answer;
static auto volatile probe_hangup = &ua_hangup;
static auto volatile probe_digit = &call_send_digit;
static auto volatile probe_call_account = &call_account;
static auto volatile probe_dtmf_mode = &account_dtmfmode;

int main() {
    if (!probe_register || !probe_unregister || !probe_get_ua || !probe_get_call ||
        !probe_connect || !probe_answer || !probe_hangup || !probe_digit || !probe_call_account || !probe_dtmf_mode)
        return 1;

    axv::BaresipEventMailbox mailbox;
    axv::BaresipCallbackLifecycle lifecycle(mailbox);
    axv::BaresipSdkEventAdapter adapter(lifecycle);

#ifdef AXV_FAKE_SDK_ABI_TEST
    const auto status = probe_register(&axv_sdk_probe_callback, nullptr);
    if (status != 0) return 2;
    probe_unregister(&axv_sdk_probe_callback);
    if (adapter.active() || adapter.session() != 0) return 3;
#endif
    // The native artifact has no startup, credential or SIP side effects;
    // its purpose is to verify successful ABI compilation and symbol linkage.
    return 0;
}
