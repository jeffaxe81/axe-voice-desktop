#include "../src/call_command_authorization.h"
#include "../src/sip_event_projection.h"
#include <memory>
#include <string_view>

namespace {
struct FakeEngine final : axv::SipEngine {
    axv::SipStatus state = axv::SipStatus::Unavailable;
    axv::SipStatus status() const override { return state; }
    bool configure(const axv::SipAccount&) override { return false; }
    bool register_account() override { return false; }
    bool start_call(const std::wstring&) override { return false; }
    void hangup() override {}
    std::wstring diagnostic() const override { return L"Synthetic only"; }
};
}

int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    SipEventProjection projection;
    EngineSelection native{std::make_unique<FakeEngine>(), EngineKind::Baresip};
    auto* backend = static_cast<FakeEngine*>(native.engine.get());
    EngineSelection unavailable = create_sip_engine();
    const auto session = mailbox.begin_session();
    auto push = [&](BaresipEventKind kind, std::uint64_t call=0) {
        return mailbox.enqueue({kind,session,call});
    };
    auto poll = [&]() -> SipRuntimeObservation {
        return projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi,
                                  mailbox.drain());
    };
    if (!push(BaresipEventKind::Registering) || !push(BaresipEventKind::RegisterOk)) return 1;
    auto state = poll();
    if (state.sip != SipStatus::Registered) return 2;
    backend->state = SipStatus::Registered;
    if (!authorize_call_command(state,session,native,CallAction::Dial,0,L"1001").allowed())
        return 3;
    auto valid = authorize_call_command(state,session,native,CallAction::Dial,0,L"ana@example.org");
    if (!valid.allowed() || valid.destination != L"sip:ana@example.org") return 4;
    if (authorize_call_command(state,session,native,CallAction::Dial,9,L"1001").reason
        != CallCommandDenied::StaleCall) return 5;
    if (authorize_call_command(state,session,native,CallAction::Dial,0,L"http://invalid").reason
        != CallCommandDenied::InvalidDestination) return 6;
    if (authorize_call_command(state,session,unavailable,CallAction::Dial,0,L"1001").reason
        != CallCommandDenied::EngineUnavailable) return 7;
    if (!push(BaresipEventKind::CallIncoming,11)) return 8;
    state = poll();
    backend->state = SipStatus::Ringing;
    if (state.call_id != 11 || state.last_call_id != 11) return 9;
    if (!authorize_call_command(state,session,native,CallAction::Answer,11).allowed()) return 10;
    if (!authorize_call_command(state,session,native,CallAction::Hangup,11).allowed()) return 11;
    if (authorize_call_command(state,session,native,CallAction::Answer,12).reason
        != CallCommandDenied::StaleCall) return 12;
    if (authorize_call_command(state,session,native,CallAction::Answer,0).reason
        != CallCommandDenied::StaleCall) return 13;
    if (authorize_call_command(state,session,native,CallAction::Answer,11,L"1001").destination.size())
        return 14; // no destination is forwarded to answer/hangup
    if (authorize_call_command(state,session,native,CallAction::Dial,0,L"1001").allowed())
        return 15;
    if (authorize_call_command(state,session+1,native,CallAction::Hangup,11).reason
        != CallCommandDenied::UnverifiedSession) return 16;
    backend->state = SipStatus::Registered;
    if (authorize_call_command(state,session,native,CallAction::Hangup,11).reason
        != CallCommandDenied::EngineStateMismatch) return 17;
    backend->state = SipStatus::Ringing;
    if (!push(BaresipEventKind::CallEstablished,11)) return 18;
    state = poll();
    backend->state = SipStatus::InCall;
    if (!authorize_call_command(state,session,native,CallAction::Hangup,11).allowed()) return 19;
    if (authorize_call_command(state,session,native,CallAction::Answer,11).reason
        != CallCommandDenied::UnsupportedState) return 20;
    if (!push(BaresipEventKind::CallClosed,11) ||
        !push(BaresipEventKind::CallOutgoing,12)) return 21;
    state = poll();
    backend->state = SipStatus::Calling;
    if (state.call_id != 12 || state.last_call_id != 12) return 22;
    if (authorize_call_command(state,session,native,CallAction::Hangup,11).reason
        != CallCommandDenied::StaleCall) return 23;
    if (!authorize_call_command(state,session,native,CallAction::Hangup,12).allowed()) return 24;
    if (!push(BaresipEventKind::CallClosed,12)) return 25;
    state = poll();
    backend->state = SipStatus::Registered;
    if (authorize_call_command(state,session,native,CallAction::Hangup,12).reason
        != CallCommandDenied::UnsupportedState) return 26;
    if (state.call_id || state.last_call_id != 12) return 27;
    if (!authorize_call_command(state,session,native,CallAction::Dial,0,L"1001").allowed())
        return 28;
    // Stopping, rejected provenance, and process failure must all revoke controls.
    state.process = RuntimeProcess::Stopping;
    if (authorize_call_command(state,session,native,CallAction::Dial,0,L"1001").reason
        != CallCommandDenied::UnverifiedSession) return 29;
    state.process = RuntimeProcess::Running;
    state.evidence = SipEvidence::None;
    if (authorize_call_command(state,session,native,CallAction::Hangup,12).allowed()) return 30;
    if (std::wstring_view(call_command_denial_text(CallCommandDenied::StaleCall)).find(L"006")
        == std::wstring_view::npos) return 31;
    // 100 attempts with stale call tokens must not ever obtain permission.
    state.evidence = SipEvidence::VerifiedNativeApi;
    state.sip = SipStatus::Ringing;
    state.call_id = 500;
    state.last_call_id = 500;
    backend->state = SipStatus::Ringing;
    for (std::uint64_t i=1; i<=100; ++i) {
        if (authorize_call_command(state,session,native,CallAction::Answer,i).allowed())
            return 32;
    }
    return 0;
}
