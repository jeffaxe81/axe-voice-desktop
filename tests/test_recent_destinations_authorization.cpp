#include "../src/recent_destinations.h"
#include "../src/dial_authorization.h"
#include <memory>
#include <string>

namespace {
struct FakeEngine final : axv::SipEngine {
    axv::SipStatus state = axv::SipStatus::Registered;
    unsigned requests = 0;
    std::wstring last_destination;
    axv::SipStatus status() const override { return state; }
    bool configure(const axv::SipAccount&) override { return false; }
    bool register_account() override { return false; }
    bool start_call(const std::wstring& destination) override {
        ++requests;
        last_destination = destination;
        return true;
    }
    void hangup() override {}
    std::wstring diagnostic() const override { return L"Synthetic authorization test"; }
};
}

int main() {
    using namespace axv;
    constexpr std::uint64_t session = 71;
    const SipRuntimeObservation registered{RuntimeProcess::Running, SipStatus::Registered,
        SipEvidence::VerifiedNativeApi, session};
    RecentDestinations recents;
    recents.synchronize(registered);
    if (!recents.remember(registered, L"1001", true) || recents.size() != 1) return 1;
    const auto* record = recents.at(0);
    if (!record) return 2;
    const auto token = record->token;
    EngineSelection selected{std::make_unique<FakeEngine>(), EngineKind::Baresip};
    auto* engine = static_cast<FakeEngine*>(selected.engine.get());

    // Choosing a row only copies a destination into the editable field.
    const auto* chosen = recents.find(token, registered);
    if (!chosen) return 3;
    std::wstring field = chosen->destination;
    if (field != L"1001" || engine->requests != 0) return 4;
    const auto manual_chamar = [&](const SipRuntimeObservation& observation,
                                    std::uint64_t current_session) {
        const auto permission = authorize_dial(observation, current_session, selected, field);
        if (permission.allowed()) selected.engine->start_call(permission.normalized_destination);
        return permission;
    };
    // A remembered request and selected field confer no registration proof.
    for (const auto status : {SipStatus::Calling, SipStatus::Ringing, SipStatus::InCall,
                              SipStatus::Failed, SipStatus::Disconnected,
                              SipStatus::Registering, SipStatus::Unavailable}) {
        auto observation = registered;
        observation.sip = status;
        if (manual_chamar(observation, session).reason != DialDenied::UnverifiedSession)
            return 5;
    }
    for (const auto process : {RuntimeProcess::Stopped, RuntimeProcess::Stopping,
                               RuntimeProcess::Failed}) {
        auto observation = registered;
        observation.process = process;
        if (manual_chamar(observation, session).reason != DialDenied::UnverifiedSession)
            return 6;
    }
    auto unverified = registered;
    unverified.evidence = SipEvidence::None;
    if (manual_chamar(unverified, session).reason != DialDenied::UnverifiedSession) return 7;
    if (manual_chamar(registered, session + 1).reason != DialDenied::UnverifiedSession) return 8;
    if (manual_chamar(registered, 0).reason != DialDenied::UnverifiedSession) return 9;
    auto stale = registered;
    stale.session = session - 1;
    if (manual_chamar(stale, session).reason != DialDenied::UnverifiedSession) return 10;
    for (const auto status : {SipStatus::Calling, SipStatus::Ringing, SipStatus::InCall,
                              SipStatus::Failed, SipStatus::Unavailable}) {
        engine->state = status;
        if (manual_chamar(registered, session).reason != DialDenied::EngineNotRegistered)
            return 11;
    }
    engine->state = SipStatus::Registered;
    selected.kind = EngineKind::Unavailable;
    if (manual_chamar(registered, session).reason != DialDenied::EngineUnavailable) return 12;
    selected.kind = EngineKind::Baresip;
    if (engine->requests != 0) return 13;
    if (!manual_chamar(registered, session).allowed() || engine->requests != 1 ||
        engine->last_destination != field) return 14;

    // Losing session proof revokes a row token, including after proof returns.
    recents.synchronize(unverified);
    if (recents.find(token, unverified) || recents.size() != 0) return 15;
    recents.synchronize(registered);
    if (recents.find(token, registered)) return 16;
    if (!recents.remember(registered, L"1001", true)) return 17;
    if (recents.find(token, registered)) return 18;
    // A copied value survives as ordinary text and is checked again on every click.
    if (manual_chamar(unverified, session).allowed() || engine->requests != 1) return 19;
    selected.engine.reset();
    if (manual_chamar(registered, session).reason != DialDenied::EngineUnavailable) return 20;
    return 0;
}
