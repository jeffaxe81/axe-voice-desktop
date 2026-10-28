#include "../src/recent_destinations.h"
#include "../src/diagnostic_export.h"
#include <iostream>
#include <string>
#include <vector>

namespace {
int failures = 0;
void check(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
axv::SipRuntimeObservation verified() {
    return {axv::RuntimeProcess::Running, axv::SipStatus::Registered,
            axv::SipEvidence::VerifiedNativeApi, 71, 0};
}
}
int main() {
    using namespace axv;
    auto state = verified();
    RecentDestinations history;
    const CallReadiness readiness{true, true, true, true, true, false, true};
    const auto diagnostic_before = format_diagnostic_export(readiness);
    const std::vector<std::wstring> invalid{
        L"", L"https://private.example/secret", L"tel:1001", L"sip:",
        L"sip:alice@", L"sip:alice@@example.org", L"sip:alice@example.org:0",
        L"sip:alice@example.org:65536", L"sip:alice@example.org?Subject=secret",
        L"sip:alice@example.org;password=secret", L"<sip:alice@example.org>",
        L"sip:alice:password@example.org", L"alice:password@example.org",
        L"sip:alice%40private@example.org", L"sip:alice%0d%0aInjected@example.org",
        L"sip:alice%00private@example.org", L"sip:alice%3btransport%3dudp@example.org",
        L"sip:alice@example.org;transport=udp", L"sip:alice@example.org?password=secret",
        L"sip:al\u00efce@example.org", L"sip:alice@ex\u00e4mple.org", L"\uff11\uff10\uff10\uff11",
        L"sip:alice!private@example.org", L"sip:alice/private@example.org",
        L"sip:alice@example.org\r\nAuthorization: secret", L"1001\t1002",
        std::wstring(L"1001\0secret", 11), std::wstring(129, L'1')};
    for (const auto& destination : invalid) {
        check(!history.remember(state, destination, true), "invalid input is rejected");
        check(history.size() == 0, "invalid input leaves no history entry");
    }
    for (int control = 0; control < 32; ++control) {
        std::wstring destination = L"100";
        destination += static_cast<wchar_t>(control);
        destination += L"1";
        check(!history.remember(state, destination, true), "ASCII control is rejected");
    }
    check(!history.remember(state, L"1001", false), "backend rejection cannot record destination");
    check(history.size() == 0, "backend rejection leaves no history");
    check(history.remember(state, L"private-user@private.example", true), "valid URI is recorded");
    check(history.size() == 1 && history.at(0) &&
          history.at(0)->destination == L"private-user@private.example", "original valid URI text is preserved");
    const auto token = history.at(0)->token;
    check(token != 0 && history.find(token, state), "current token resolves");
    check(!history.find(0, state), "zero token cannot resolve");
    check(history.remember(state, L"private-user@private.example", true), "duplicate can refresh");
    check(history.size() == 1 && history.at(0)->token != token, "duplicate rotates its token");
    check(!history.find(token, state), "duplicate's previous token cannot replay");
    const auto refreshed_token = history.at(0)->token;
    for (const auto& destination : invalid) {
        check(!history.remember(state, destination, true), "invalid input stays rejected with populated history");
        check(history.size() == 1 && history.find(refreshed_token, state),
              "rejected input preserves valid existing history and token");
    }
    check(!history.remember(state, L"2002", false) && history.find(refreshed_token, state),
          "backend rejection preserves existing destination");
    history.clear();
    check(!history.find(refreshed_token, state), "clear invalidates old token");
    check(history.remember(state, L"1001", true), "record after clear works");
    check(history.at(0)->token != refreshed_token && history.at(0)->token != token,
          "clear does not reset token identity");
    const auto session_token = history.at(0)->token;
    auto next_session = state;
    ++next_session.session;
    check(!history.find(session_token, next_session), "old session token cannot resolve in new session");
    history.synchronize(next_session);
    check(history.size() == 0, "session transition clears history");
    check(history.remember(next_session, L"1001", true), "new session records independently");
    check(!history.find(session_token, next_session), "new record cannot revive old session token");

    std::vector<SipRuntimeObservation> unauthorized;
    auto bad = state; bad.process = RuntimeProcess::Stopped; unauthorized.push_back(bad);
    bad = state; bad.process = RuntimeProcess::Stopping; unauthorized.push_back(bad);
    bad = state; bad.process = RuntimeProcess::Failed; unauthorized.push_back(bad);
    bad = state; bad.evidence = SipEvidence::None; unauthorized.push_back(bad);
    bad = state; bad.session = 0; unauthorized.push_back(bad);
    bad = state; bad.sip = SipStatus::Disconnected; unauthorized.push_back(bad);
    for (const auto& observation : unauthorized) {
        RecentDestinations isolated;
        check(isolated.remember(state, L"1001", true), "seed valid context");
        const auto old_token = isolated.at(0)->token;
        check(!isolated.find(old_token, observation), "unauthorized observation cannot resolve token");
        check(!isolated.remember(observation, L"2002", true), "unauthorized observation cannot record");
        isolated.synchronize(observation);
        check(isolated.size() == 0, "invalid proof removes retained destinations");
        check(!isolated.find(old_token, state), "restored proof cannot resurrect invalidated token");
    }
    for (const auto status : {SipStatus::Calling, SipStatus::Ringing, SipStatus::InCall}) {
        RecentDestinations active;
        check(active.remember(state, L"1001", true), "seed before active call");
        const auto active_token = active.at(0)->token;
        auto in_call = state;
        in_call.sip = status;
        in_call.call_id = 4;
        active.synchronize(in_call);
        check(active.size() == 1 && active.find(active_token, in_call),
              "verified active call preserves selectable history");
        check(!active.remember(in_call, L"2002", true) && active.size() == 1,
              "active call cannot record a new destination");
    }
    auto nonidle = state;
    nonidle.call_id = 4;
    RecentDestinations nonidle_history;
    check(!nonidle_history.remember(nonidle, L"1001", true),
          "registered status with active call token cannot record");
    check(format_diagnostic_export(readiness) == diagnostic_before,
          "diagnostic export is byte-for-byte unchanged after private history operations");
    check(format_diagnostic_export(readiness).find(L"private") == std::wstring::npos,
          "diagnostic export contains no private destination");
    return failures ? 1 : 0;
}
