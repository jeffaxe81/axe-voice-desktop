#include "../src/native_transfer.h"
#include "../src/destination_feedback.h"
#include <iostream>

// Always-on assertions: destination rejection remains enforced in Release.
namespace {
int failures = 0;
void check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}
}

int main() {
    const axv::SipAccount udp{L"pbx.example", L"1001", 5060, false};
    for (const auto* host : {L"", L"-bad.example", L"bad-.example",
                             L"pbx.-bad.example", L"pbx.bad-.example",
                             L"a..example", L".example", L"example."}) {
        const std::wstring uri = std::wstring(L"sip:200@") + host + L":5060";
        check(!axv::validate_address(uri), "URI parser rejects invalid hostname before explicit port");
        check(!axv::evaluate_destination(uri).valid(), "feedback does not label malformed hostname valid");
        check(axv::native_call_destination(uri, udp).empty(), "dial rejects malformed hostname");
        check(axv::native_transfer_destination(uri, udp).empty(), "transfer rejects malformed hostname");
        check(!axv::valid_native_transfer_uri(axv::native_ascii(uri)), "REFER boundary rejects malformed hostname");
        auto account = udp;
        account.server = host;
        check(axv::validate_account(account) != axv::AccountError::None, "account rejects invalid DNS labels");
        check(!axv::native_account_config(account, L"").valid, "native account rejects invalid DNS labels");
    }
    for (const auto* host : {L"pbx.example", L"pbx-1.example", L"a.b", L"localhost", L"127.0.0.1"}) {
        const std::wstring uri = std::wstring(L"sip:200@") + host + L":5060";
        check(static_cast<bool>(axv::validate_address(uri)), "URI parser accepts supported DNS and IPv4 hosts");
        check(axv::evaluate_destination(uri).valid(), "feedback accepts supported hostname");
        check(axv::native_call_destination(uri, udp) == axv::native_ascii(uri), "dial preserves supported hostname");
        check(axv::native_transfer_destination(uri, udp) == axv::native_ascii(uri), "transfer preserves supported hostname");
        auto account = udp;
        account.server = host;
        check(axv::validate_account(account) == axv::AccountError::None, "account accepts supported DNS and IPv4 hosts");
        check(axv::native_account_config(account, L"").valid, "native account accepts supported hostname");
    }
    const auto label63=std::wstring(63,L'a');
    const auto host253=label63+L"."+label63+L"."+label63+L"."+std::wstring(61,L'b');
    check(bool(axv::validate_address(L"sip:200@"+host253)),"253-character hostname accepted");
    check(!axv::validate_address(L"sip:200@"+host253+L"x"),"254-character hostname rejected");
    check(!axv::validate_address(L"sip:200@"+std::wstring(64,L'a')+L".example"),"64-character label rejected");
    return failures ? 1 : 0;
}
