#include "../src/account_form_feedback.h"
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
    for (const auto* url : {L"https://pbx.example.org/", L"http://pbx.example.org", L"pbx.example.org/"}) {
        result = evaluate_account_form(url, L"1001", L"5060");
        if (result.kind != AccountFeedbackKind::Invalid || result.error != AccountError::InvalidServer ||
            std::wstring(result.message).find(L"somente o host") == std::wstring::npos) return 10;
    }
    result = evaluate_account_form(L"pbx.example.org", L"a@b", L"5060");
    if (result.kind != AccountFeedbackKind::Invalid ||
        result.error != AccountError::InvalidUser) return 6;
    result = evaluate_account_form(L"pbx.example.org", L"1001", L"5060");
    if (!result.valid() || result.port != 5060 || result.error != AccountError::None) return 7;
    if (std::wstring(result.message).find(L"Registro SIP nao realizado") == std::wstring::npos) return 8;
    result = evaluate_account_form(L"192.168.40.84", L"1001", L"65535");
    if (!result.valid() || result.port != 65535) return 9;
    return 0;
}
