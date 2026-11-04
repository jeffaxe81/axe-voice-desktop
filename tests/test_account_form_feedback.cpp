#include "../src/account_form_feedback.h"
#include "../src/sip_transport_port.h"
#include <string>

int main() {
    using namespace axv;
    auto result = evaluate_account_form(L"", L"", L"");
    if (result.kind != AccountFeedbackKind::Incomplete ||
        result.error != AccountError::MissingServer) return 1;
    result = evaluate_account_form(L"pbx.example.org", L"", L"5060");
    if (result.kind != AccountFeedbackKind::Incomplete ||
        result.error != AccountError::MissingUser) return 2;
    result = evaluate_account_form(L"pbx.example.org", L"1001", L"");
    if (result.kind != AccountFeedbackKind::Incomplete) return 3;
    for (const auto* bad_port : {L"0", L"65536", L"9999999999999999", L"50A0", L" 5060", L"-1"}) {
        result = evaluate_account_form(L"pbx.example.org", L"1001", bad_port);
        if (result.kind != AccountFeedbackKind::Invalid ||
            result.error != AccountError::InvalidPort || result.valid()) return 4;
    }
    result = evaluate_account_form(L"bad host", L"1001", L"5060");
    if (result.kind != AccountFeedbackKind::Invalid ||
        result.error != AccountError::InvalidServer) return 5;
    result = evaluate_account_form(L"pbx.example.org", L"a@b", L"5060");
    if (result.kind != AccountFeedbackKind::Invalid ||
        result.error != AccountError::InvalidUser) return 6;
    result = evaluate_account_form(L"pbx.example.org", L"1001", L"5060");
    if (!result.valid() || result.port != 5060 || result.error != AccountError::None) return 7;
    if (std::wstring(result.message).find(L"Registro SIP nao realizado") == std::wstring::npos) return 8;
    result = evaluate_account_form(L"192.168.40.84", L"1001", L"65535");
    if (!result.valid() || result.port != 65535) return 9;
    for (const auto* server : {L"10.0.0.1", L"192.168.40.84", L"pbx.example.com",
                               L"sip.empresa.com.br", L"pbx-local"}) {
        result = evaluate_account_form(server, L"1001", L"5060");
        if (!result.valid()) return 10;
    }
    for (const auto* server : {L"256.1.1.1", L"192.168.1.999", L"1.2.3",
                               L"1.2.3.4.5", L"pbx.example.com:5060",
                               L"sip:pbx.example.com", L"https://pbx.example.com",
                               L"host..example.com", L"-pbx.example.com"}) {
        result = evaluate_account_form(server, L"1001", L"5060");
        if (result.valid() || result.error != AccountError::InvalidServer) return 11;
    }
    if (std::wstring_view(suggested_sip_port(true,L"5060")) != L"5061") return 12;
    if (std::wstring_view(suggested_sip_port(false,L"5061")) != L"5060") return 13;
    if (std::wstring_view(suggested_sip_port(true,L"")) != L"5061") return 14;
    if (std::wstring_view(suggested_sip_port(false,L"")) != L"5060") return 15;
    if (suggested_sip_port(true,L"5070") != nullptr) return 16;
    if (suggested_sip_port(false,L"5080") != nullptr) return 17;
    return 0;
}
