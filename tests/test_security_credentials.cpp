#include "../src/account_form_feedback.h"
#include "../src/native_account_config.h"
#include "../src/diagnostic_export.h"
#include <iostream>
#include <string>

namespace {
int failures = 0;
void check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; ++failures; }
}
}

int main() {
    // Synthetic account values only. Native configuration must reject syntax
    // capable of introducing account attributes before anything reaches SDK.
    for (const auto* user : {L"1001;auth_pass=SYNTHETIC-ONLY",
                             L"1001\r\nsip_verify_server no",
                             L"1001\nSYNTHETIC-ONLY", L"1001\""}) {
        axv::SipAccount account{L"pbx.example", user, 5061, true};
        const auto config = axv::native_account_config(account, L"test-ca.pem");
        check(!config.valid && config.aor.empty() && config.config.empty(),
              "unsafe username reached native account configuration");
        check(!axv::evaluate_account_form(account.server, account.username, L"5061").valid(),
              "account form accepts account-configuration metacharacters");
    }
    axv::SipAccount account{L"pbx.example", L"1001", 5061, true};
    for (const auto* ca : {L"test-ca.pem\nsip_verify_server no",
                          L"test-ca.pem\rsip_verify_server no"}) {
        check(!axv::native_account_config(account, ca).valid,
              "CA field permits a second SDK configuration directive");
    }
    for (const auto* user : {L"1001", L"agent_1", L"agent-1", L"agent.1", L"+1001"}) {
        account.username = user;
        check(axv::evaluate_account_form(account.server, account.username, L"5061").valid(),
              "supported account rejected by form");
        const auto config = axv::native_account_config(account, L"test-ca.pem");
        check(config.valid && config.config.find("sip_verify_server yes\n") != std::string::npos,
              "supported TLS account lacks server verification");
    }
    // Exercise every boolean diagnostic state, including unusual combinations.
    // The export must stay suitable for copying without identifiers or secrets.
    for (unsigned bits = 0; bits < 128; ++bits) {
        const axv::CallReadiness state{bool(bits&1),bool(bits&2),bool(bits&4),
            bool(bits&8),bool(bits&16),bool(bits&32),bool(bits&64)};
        const auto text = axv::format_diagnostic_export(state);
        check(!text.empty(), "empty diagnostic export");
        for (const auto* forbidden : {L"pbx.example", L"SYNTHETIC-ONLY", L"auth_pass=",
                                      L"sip:", L"sips:", L"@", L"test-ca.pem"})
            check(text.find(forbidden) == std::wstring::npos,
                  "diagnostic export contains account or credential material");
        check(text.find(L'\0') == std::wstring::npos, "embedded NUL in clipboard text");
        for (std::size_t i = 0; i < text.size(); ++i) {
            if (text[i] == L'\n') check(i && text[i-1] == L'\r', "bare LF in clipboard text");
            if (text[i] == L'\r') check(i+1 < text.size() && text[i+1] == L'\n', "bare CR in clipboard text");
        }
    }
    return failures ? 1 : 0;
}
