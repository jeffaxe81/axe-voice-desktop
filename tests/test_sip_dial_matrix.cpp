#include "../src/dial_authorization.h"
#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace {
class TestEngine final : public axv::SipEngine {
public:
    axv::SipStatus observed = axv::SipStatus::Unavailable;
    axv::SipStatus status() const override { return observed; }
    bool configure(const axv::SipAccount&) override { return false; }
    bool register_account() override { return false; }
    bool start_call(const std::wstring&) override { return false; }
    void hangup() override {}
    std::wstring diagnostic() const override { return L"Test double only"; }
};
}

int main() {
    using namespace axv;
    constexpr std::uint64_t session = 50;
    constexpr std::array processes{
        RuntimeProcess::Stopped, RuntimeProcess::Running,
        RuntimeProcess::Stopping, RuntimeProcess::Failed
    };
    constexpr std::array evidence{SipEvidence::None, SipEvidence::VerifiedNativeApi};
    constexpr std::array statuses{
        SipStatus::Unavailable, SipStatus::Disconnected,
        SipStatus::Registering, SipStatus::Registered,
        SipStatus::Calling, SipStatus::Ringing,
        SipStatus::InCall, SipStatus::Failed
    };
    constexpr std::array<std::uint64_t,4> session_checks{0, session - 1, session, session + 1};
    constexpr std::array kinds{EngineKind::Unavailable, EngineKind::Baresip};
    constexpr std::array engine_states{
        SipStatus::Unavailable, SipStatus::Registering, SipStatus::Registered
    };
    const std::array<std::wstring,4> destinations{
        L"", L"100 1", L"1001", L"ana@example.org"
    };

    EngineSelection test_engine{std::make_unique<TestEngine>(), EngineKind::Unavailable};
    auto* synthetic = static_cast<TestEngine*>(test_engine.engine.get());
    std::size_t checked = 0;
    std::size_t allowed = 0;
    for (const auto process : processes)
    for (const auto proof : evidence)
    for (const auto sip : statuses)
    for (const auto current : session_checks)
    for (const auto kind : kinds)
    for (const auto engine_sip : engine_states)
    for (std::size_t address = 0; address < destinations.size(); ++address) {
        const SipRuntimeObservation observed{process, sip, proof, session};
        test_engine.kind = kind;
        synthetic->observed = engine_sip;
        const auto result = authorize_dial(observed, current, test_engine,
                                           destinations[address]);
        const bool expected =
            process == RuntimeProcess::Running &&
            proof == SipEvidence::VerifiedNativeApi &&
            sip == SipStatus::Registered &&
            current == session &&
            kind == EngineKind::Baresip &&
            engine_sip == SipStatus::Registered &&
            address >= 2;
        if (result.allowed() != expected) return 1;
        if (!expected && !result.normalized_destination.empty()) return 2;
        if (expected) {
            const auto normalized = address == 2 ? L"1001" : L"sip:ana@example.org";
            if (result.normalized_destination != normalized) return 3;
            ++allowed;
        }
        ++checked;
    }
    if (checked != 6144 || allowed != 2) return 4;

    // The real frontend's factory is not the synthetic engine above.
    // A forged "registered" model cannot enable Dial against its fallback.
    auto unavailable = create_sip_engine();
    if (unavailable.kind != EngineKind::Unavailable || !unavailable.engine)
        return 5;
    for (const auto& dest : {L"1001", L"sip:ana@example.org"}) {
        const SipRuntimeObservation forged{RuntimeProcess::Running,
            SipStatus::Registered, SipEvidence::VerifiedNativeApi, session};
        const auto result = authorize_dial(forged, session, unavailable, dest);
        if (result.allowed() || result.reason != DialDenied::EngineUnavailable)
            return 6;
    }
    if (unavailable.engine->register_account() ||
        unavailable.engine->start_call(L"1001")) return 7;
    return 0;
}
