#include "../src/native_transfer.h"
#include <iostream>
#include <string>

namespace {
int failures = 0;
void check(bool condition, const char* description) {
    if (!condition) {
        std::cerr << "FAIL: " << description << '\n';
        ++failures;
    }
}
void rejected(const std::wstring& input, const axv::SipAccount& account,
              axv::NativeDestinationError expected) {
    const auto call = axv::native_call_destination_result(input, account);
    const auto transfer = axv::native_transfer_destination_result(input, account);
    check(call.error == expected && call.uri.empty(), "dial returns precise refusal without URI");
    check(transfer.error == expected && transfer.uri.empty(), "transfer returns precise refusal without URI");
    check(axv::native_call_destination(input, account).empty(), "dial string wrapper preserves refusal");
    check(axv::native_transfer_destination(input, account).empty(), "transfer string wrapper preserves refusal");
    const std::wstring message = axv::native_destination_error_message(call.error);
    check(!message.empty(), "refusal has actionable static feedback");
    check(input.empty() || message.find(input) == std::wstring::npos, "refusal does not echo destination");
}
}

int main() {
    const axv::SipAccount udp{L"pbx.example", L"1001", 5060, false};
    const axv::SipAccount tls{L"pbx.example", L"1001", 5061, true};
    for (const auto& account : {udp, tls}) {
        const auto expected = account.use_tls
            ? "sips:200@pbx.example:5061;transport=tls"
            : "sip:200@pbx.example:5060;transport=udp";
        const auto call = axv::native_call_destination_result(L"200", account);
        const auto transfer = axv::native_transfer_destination_result(L"200", account);
        check(call.error == axv::NativeDestinationError::None && call.uri == expected,
              "valid extension retains account routing");
        check(transfer.error == axv::NativeDestinationError::None && transfer.uri == expected,
              "valid transfer retains account routing");
        check(axv::native_call_destination(L"200", account) == call.uri,
              "dial wrapper preserves successful result");
        check(axv::native_transfer_destination(L"200", account) == transfer.uri,
              "transfer wrapper preserves successful result");
    }
    const std::wstring explicit_uri = L"sip:user.name+1@other.example:5070";
    check(axv::native_call_destination_result(explicit_uri, udp).uri == axv::native_ascii(explicit_uri),
          "explicit supported destination stays unchanged");
    rejected(L"", udp, axv::NativeDestinationError::InvalidAddress);
    rejected(L"sip:priv\u00e9@pbx.example", udp, axv::NativeDestinationError::UnsupportedCharacters);
    rejected(L"sips:private-marker@pbx.example", udp, axv::NativeDestinationError::TransportMismatch);
    rejected(L"sip:private-marker@pbx.example", tls, axv::NativeDestinationError::TransportMismatch);
    rejected(L"sip:private!marker@pbx.example", udp, axv::NativeDestinationError::UnsupportedCharacters);
    rejected(L"sip:private-marker@-bad.example", udp, axv::NativeDestinationError::InvalidAddress);
    auto invalid = udp;
    invalid.server = L"private-invalid-account";
    invalid.username.clear();
    rejected(L"987654321", invalid, axv::NativeDestinationError::InvalidAccount);

    // Both inputs fit the parser's input bound; account expansion crosses the final byte bound.
    const std::string routing_suffix = "@pbx.example:5060;transport=udp";
    const std::wstring maximum_bare(512 - 4 - routing_suffix.size(), L'7');
    const auto boundary = axv::native_call_destination_result(maximum_bare, udp);
    check(boundary.error == axv::NativeDestinationError::None && boundary.uri.size() == 512,
          "exact final 512-byte routed URI accepted");
    check(axv::native_transfer_destination_result(maximum_bare, udp).uri == boundary.uri,
          "transfer accepts final 512-byte boundary");
    rejected(maximum_bare + L"7", udp, axv::NativeDestinationError::TooLong);
    rejected(std::wstring(513, L'7'), udp, axv::NativeDestinationError::TooLong);
    const std::wstring explicit512 = L"sip:" + std::wstring(496, L'8') + L"@pbx.example";
    check(axv::native_call_destination_result(explicit512, udp).uri.size() == 512,
          "explicit 512-byte URI accepted");
    rejected(explicit512 + L"x", udp, axv::NativeDestinationError::TooLong);

    // Error-only message lookup cannot depend on account or destination content.
    const auto first = axv::native_call_destination_result(L"sip:first!private@pbx.example", udp);
    const auto second = axv::native_call_destination_result(L"sip:second!secret@pbx.example", udp);
    check(std::wstring(axv::native_destination_error_message(first.error)) ==
              axv::native_destination_error_message(second.error),
          "same reason produces identical privacy-safe feedback");
    return failures ? 1 : 0;
}
