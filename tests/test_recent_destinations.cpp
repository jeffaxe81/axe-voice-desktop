#include "../src/recent_destinations.h"
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>

// Deliberately active under NDEBUG: these are also Release safety checks.
#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "recent destinations check failed at line %d: %s\n", __LINE__, #condition); \
    std::abort(); } } while (false)

namespace {
axv::SipRuntimeObservation ready(std::uint64_t session = 41) {
    return {axv::RuntimeProcess::Running, axv::SipStatus::Registered,
            axv::SipEvidence::VerifiedNativeApi, session};
}
}

int main() {
    using namespace axv;
    const auto current = ready();
    RecentDestinations recents;
    CHECK(RecentDestinations::capacity == 10);
    CHECK(recents.size() == 0 && recents.at(0) == nullptr);
    CHECK(recents.find(0, current) == nullptr);
    CHECK(recents.remember(current, L"1001", true));
    CHECK(recents.size() == 1 && recents.at(0)->destination == L"1001");
    const auto first = recents.at(0)->token;
    CHECK(first != 0 && recents.find(first, current) != nullptr);
    CHECK(!recents.remember(current, L"1002", false));
    CHECK(recents.size() == 1 && recents.at(0)->token == first);
    CHECK(recents.remember(current, L"sip:ana@example.org", true));
    const auto second = recents.at(0)->token;
    CHECK(second != first && second != 0);
    CHECK(recents.remember(current, L"1001", true));
    CHECK(recents.size() == 2 && recents.at(0)->destination == L"1001");
    CHECK(recents.at(1)->destination == L"sip:ana@example.org");
    CHECK(recents.at(0)->token != first && recents.at(0)->token != second);
    CHECK(recents.find(first, current) == nullptr);
    CHECK(recents.find(second, current) != nullptr);

    const auto unchanged = recents.at(0)->token;
    const std::wstring rejected[] = {L"", L" 1001", L"1001\n",
        L"https://example.org", L"sip:ana@example.org;transport=tcp", L"sip:a@bad host",
        std::wstring(129, L'1'), std::wstring(L"1001\0hidden", 11)};
    for (const auto& destination : rejected) {
        CHECK(!recents.remember(current, destination, true));
        CHECK(recents.size() == 2 && recents.at(0)->token == unchanged);
    }
    CHECK(recents.remember(current, std::wstring(128, L'1'), true));
    CHECK(recents.at(0)->destination.size() == 128);
    CHECK(recents.remember(current, L"ana@example.org", true));
    CHECK(recents.at(0)->destination == L"ana@example.org");
    // A valid typed URI at the edit limit must remain selectable verbatim,
    // even when normalizing it would add four characters for the SIP scheme.
    const std::wstring long_typed = std::wstring(116, L'a') + L"@example.org";
    CHECK(long_typed.size() == 128);
    CHECK(recents.remember(current, long_typed, true));
    CHECK(recents.at(0)->destination == long_typed);
    CHECK(recents.remember(current, L"sips:Ana@example.org", true));
    CHECK(recents.remember(current, L"sips:ana@example.org", true));
    CHECK(recents.at(1)->destination == L"sips:Ana@example.org");

    recents.clear();
    CHECK(recents.size() == 0 && recents.at(0) == nullptr);
    CHECK(recents.find(second, current) == nullptr);
    std::set<std::uint64_t> tokens{first, second, unchanged};
    for (unsigned index = 0; index != 14; ++index) {
        CHECK(recents.remember(current, std::to_wstring(2000 + index), true));
        CHECK(tokens.insert(recents.at(0)->token).second);
    }
    CHECK(recents.size() == 10 && recents.at(10) == nullptr);
    for (unsigned index = 0; index != 10; ++index)
        CHECK(recents.at(index)->destination == std::to_wstring(2013 - index));

    const auto selected = recents.at(0)->token;
    for (auto status : {SipStatus::Calling, SipStatus::Ringing, SipStatus::InCall}) {
        auto during_call = current;
        during_call.sip = status;
        during_call.call_id = 88;
        recents.synchronize(during_call);
        CHECK(recents.size() == 10);
        CHECK(!recents.remember(during_call, L"9999", true));
    }
    recents.synchronize(current);
    CHECK(recents.find(selected, current) != nullptr);
    auto with_call = current;
    with_call.call_id = 9;
    CHECK(!recents.remember(with_call, L"9999", true));

    for (unsigned scenario = 0; scenario != 10; ++scenario) {
        recents.clear();
        CHECK(recents.remember(current, L"1001", true));
        const auto token = recents.at(0)->token;
        auto invalid = current;
        switch (scenario) {
        case 0: invalid.process = RuntimeProcess::Stopped; break;
        case 1: invalid.process = RuntimeProcess::Stopping; break;
        case 2: invalid.process = RuntimeProcess::Failed; break;
        case 3: invalid.evidence = SipEvidence::None; break;
        case 4: invalid.session = 0; break;
        case 5: invalid.session++; break;
        case 6: invalid.sip = SipStatus::Failed; break;
        case 7: invalid.sip = SipStatus::Disconnected; break;
        case 8: invalid.sip = SipStatus::Registering; break;
        case 9: invalid.sip = SipStatus::Unavailable; break;
        }
        CHECK(recents.find(token, invalid) == nullptr);
        recents.synchronize(invalid);
        CHECK(recents.size() == 0);
        CHECK(recents.find(token, current) == nullptr);
    }
    for (auto status : {SipStatus::Unavailable, SipStatus::Registering,
                        SipStatus::Failed, SipStatus::Disconnected}) {
        auto unavailable = current;
        unavailable.sip = status;
        CHECK(!recents.remember(unavailable, L"1001", true));
    }
    CHECK(recents.remember(ready(42), L"1002", true));
    CHECK(recents.size() == 1 && recents.at(0)->destination == L"1002");
    CHECK(recents.find(recents.at(0)->token, current) == nullptr);
    return 0;
}
