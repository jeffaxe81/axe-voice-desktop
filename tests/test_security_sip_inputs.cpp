#include "../src/native_transfer.h"
#include "../src/native_dtmf.h"
#include "../src/baresip_profile.h"
#include <iostream>
#include <string>

// Always-on assertions: this regression must also run with -DNDEBUG.
namespace {
int failures = 0;
void check(bool condition, const char* description) {
    if (!condition) {
        std::cerr << "FAIL: " << description << '\n';
        ++failures;
    }
}
bool secure_or_rejected(const std::string& destination) {
    return destination.empty() ||
        (destination.compare(0, 5, "sips:") == 0 &&
         destination.find("transport=udp") == std::string::npos);
}
}

int main() {
    const axv::SipAccount udp{L"pbx.example", L"1001", 5060, false};
    const axv::SipAccount tls{L"pbx.example", L"1001", 5061, true};
    check(axv::native_call_destination(L"200", udp) ==
          "sip:200@pbx.example:5060;transport=udp", "ordinary UDP dialing works");
    check(axv::native_call_destination(L"200", tls) ==
          "sips:200@pbx.example:5061;transport=tls", "ordinary TLS dialing works");
    check(axv::validate_address(L"sip:200").normalized == L"sip:200",
          "normalization preserves explicit SIP scheme on bare dial string");
    check(axv::validate_address(L"sips:200").normalized == L"sips:200",
          "normalization preserves explicit SIPS scheme on bare dial string");
    check(axv::native_call_destination(L"sips:200", tls) ==
          "sips:200@pbx.example:5061;transport=tls",
          "TLS account routes explicit SIPS bare dial without duplicate scheme");
    check(axv::native_call_destination(L"sip:200", udp) ==
          "sip:200@pbx.example:5060;transport=udp",
          "UDP account routes explicit SIP bare dial without duplicate scheme");
    check(axv::native_call_destination(L"sips:200@pbx.example", udp).empty(),
          "UDP-only backend rejects full explicit SIPS destination");
    check(axv::native_call_destination(L"sips:200", udp).empty(),
          "UDP-only backend rejects bare explicit SIPS destination");
    check(axv::native_call_destination(L"sip:200", tls).empty(),
          "TLS-only account rejects explicit SIP bare dial downgrade");

    // Explicit SIPS is a security requirement even when account routing supplies
    // the missing host. Rejecting this shorthand is also a safe implementation.
    check(secure_or_rejected(axv::native_call_destination(L"sips:200", udp)),
          "explicit SIPS bare destination must not silently become UDP");
    check(secure_or_rejected(axv::native_transfer_destination(L"sips:200", udp)),
          "explicit SIPS transfer must not silently become UDP");

    for (const auto& bad : {L"sip:200@pbx.example\r\nX-Test: injected",
                            L"sip:200@pbx.example?Subject=injected",
                            L"sip:200@pbx.example;transport=udp"}) {
        check(axv::native_call_destination(bad, udp).empty(),
              "call destination rejects raw header/config injection");
        check(axv::native_transfer_destination(bad, udp).empty(),
              "transfer destination rejects raw header/config injection");
    }
    check(axv::native_call_destination(std::wstring(L"sip:200\0evil@pbx.example", 24), udp).empty(),
          "embedded NUL cannot truncate call destination");
    for (unsigned int key = 0; key < 256; ++key) {
        const bool expected = (key >= '0' && key <= '9') || key == '*' || key == '#';
        check(axv::valid_native_dtmf_key(static_cast<wchar_t>(key)) == expected,
              "DTMF accepts exactly decimal digits, star and hash");
    }
    check(!axv::valid_native_transfer_uri(std::string(513, 'a')),
          "transfer rejects inputs exceeding 512 bytes");
    check(!axv::valid_native_transfer_uri("sip:200%0d%0aX@pbx.example"),
          "transfer rejects percent-encoded unsafe bytes");
    // Defensive hardening: these demonstrate inconsistent boundary validation,
    // not proven header injection through a real SIP stack.
    for (const auto& encoded : {L"sip:alice%0d%0aX-Test%3ainjected@pbx.example",
                                L"sip:alice%00evil@pbx.example",
                                L"sip:alice%3btransport%3dudp@pbx.example"}) {
        check(axv::native_call_destination(encoded, udp).empty(),
              "native call rejects escaped unsafe user bytes like transfer");
    }
    check(axv::native_call_destination(L"sip:" + std::wstring(500, L'a') +
                                      L"@pbx.example", udp).empty(),
          "native call rejects explicit URI output longer than 512 bytes");
    check(axv::native_call_destination(std::wstring(500, L'2'), udp).empty(),
          "native call rejects routed bare dial output longer than 512 bytes");
    auto injected = udp;
    injected.username = L"1001;auth_pass=injected";
    check(!axv::native_account_config(injected, L"").valid,
          "native account rejects account-parameter injection");
    injected = udp;
    injected.server = L"pbx.example\nmodule ctrl_tcp.so";
    check(!axv::native_account_config(injected, L"").valid,
          "native account rejects config-line injection");
    check(axv::inspect_baresip_config_line("module ctrl_tcp.so") ==
          axv::BaresipProfileStatus::InsecureRemoteControl,
          "profile rejects remote-control module");
    check(axv::inspect_baresip_config_line("include injected.conf") ==
          axv::BaresipProfileStatus::UnsafeConfiguration,
          "profile rejects external includes");
    return failures ? 1 : 0;
}
