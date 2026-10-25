#include "../src/dial_authorization.h"
#include <memory>
#include <string_view>

namespace {
struct FakeEngine final : axv::SipEngine {
    axv::SipStatus state = axv::SipStatus::Registered;
    int calls = 0;
    std::wstring dialed;
    axv::SipStatus status() const override { return state; }
    bool configure(const axv::SipAccount&) override { return false; }
    bool register_account() override { return false; }
    bool start_call(const std::wstring& destination) override {
        ++calls;
        dialed = destination;
        return true;
    }
    void hangup() override {}
    std::wstring diagnostic() const override { return L"Simulador exclusivo do teste"; }
};
}

int main() {
    using namespace axv;
    const std::uint64_t session = 71;
    SipRuntimeObservation observed{RuntimeProcess::Running, SipStatus::Registered,
                                   SipEvidence::VerifiedNativeApi, session};
    EngineSelection selected{std::make_unique<FakeEngine>(), EngineKind::Baresip};
    auto* fake = static_cast<FakeEngine*>(selected.engine.get());
    const auto attempt = [&](const SipRuntimeObservation& state, std::uint64_t generation,
                             const std::wstring& typed) {
        const auto permit = authorize_dial(state, generation, selected, typed);
        if (permit.allowed()) selected.engine->start_call(permit.normalized_destination);
        return permit;
    };
    if (attempt(observed, session, L"").reason != DialDenied::EmptyDestination) return 1;
    if (attempt(observed, session, L"https://evil.test").reason != DialDenied::InvalidDestination) return 2;
    if (fake->calls != 0) return 3;
    observed.evidence = SipEvidence::None;
    if (attempt(observed, session, L"1001").reason != DialDenied::UnverifiedSession) return 4;
    observed.evidence = SipEvidence::VerifiedNativeApi;
    if (attempt(observed, session + 1, L"1001").reason != DialDenied::UnverifiedSession) return 5;
    observed.process = RuntimeProcess::Stopping;
    if (attempt(observed, session, L"1001").reason != DialDenied::UnverifiedSession) return 6;
    observed.process = RuntimeProcess::Running;
    observed.sip = SipStatus::Calling;
    if (attempt(observed, session, L"1001").reason != DialDenied::UnverifiedSession) return 7;
    observed.sip = SipStatus::Registered;
    fake->state = SipStatus::Disconnected;
    if (attempt(observed, session, L"1001").reason != DialDenied::EngineNotRegistered) return 8;
    fake->state = SipStatus::Registered;
    selected.kind = EngineKind::Unavailable;
    if (attempt(observed, session, L"1001").reason != DialDenied::EngineUnavailable) return 9;
    selected.kind = EngineKind::Baresip;
    if (fake->calls != 0) return 10;
    const auto allowed = attempt(observed, session, L"1001");
    if (!allowed.allowed() || fake->calls != 1 || fake->dialed != L"1001") return 11;
    const auto sip_uri = attempt(observed, session, L"ana@example.org");
    if (!sip_uri.allowed() || fake->calls != 2 || fake->dialed != L"sip:ana@example.org") return 12;
    selected.engine.reset();
    if (attempt(observed, session, L"1001").reason != DialDenied::EngineUnavailable) return 13;
    EngineSelection unavailable = create_sip_engine();
    const auto lab = authorize_dial(observed, session, unavailable, L"1001");
    if (lab.allowed() || lab.reason != DialDenied::EngineUnavailable) return 14;
    if (std::wstring_view(dial_denial_text(DialDenied::UnverifiedSession)).find(L"AXV-SIP-003")
        == std::wstring_view::npos) return 15;
    return 0;
}
