#include "../src/sip_command_executor.h"
#include <cstdint>
#include <memory>
#include <string>

struct FakeEngine final : axv::SipEngine {
    axv::SipStatus state = axv::SipStatus::Registered;
    axv::SipStatus status() const override { return state; }
    bool configure(const axv::SipAccount&) override { return false; }
    bool register_account() override { return false; }
    bool start_call(const std::wstring&) override { return false; }
    void hangup() override {}
    std::wstring diagnostic() const override { return L"synthetic"; }
};
struct FakeNative final : axv::SipNativeCommandPort {
    int outgoing = 0, answered = 0, hungup = 0;
    bool originate(const std::wstring&) override { ++outgoing; return true; }
    bool answer(std::uint64_t) override { ++answered; return true; }
    bool hangup(std::uint64_t) override { ++hungup; return true; }
};

int main() {
    using namespace axv;
    SipCommandExecutor executor;
    FakeNative native;
    EngineSelection selected{std::make_unique<FakeEngine>(), EngineKind::Baresip};
    auto* engine = static_cast<FakeEngine*>(selected.engine.get());
    SipRuntimeObservation live{RuntimeProcess::Running, SipStatus::Registered,
                               SipEvidence::VerifiedNativeApi, 101};
    auto dial = [](std::uint64_t session, std::uint64_t seq) {
        return SipCommandRequest{session, seq, CallAction::Dial, 0, L"1001"};
    };

    if (executor.watchdog(live, 101, 1000) != SipCommandDisposition::Idle)
        return 1;
    if (executor.dispatch(dial(101,1), live,101,selected,native,0).disposition
        != SipCommandDisposition::InvalidRequest) return 2;
    if (executor.dispatch(dial(101,1),live,101,selected,native,5000).disposition
        != SipCommandDisposition::Submitted || native.outgoing != 1) return 3;
    if (executor.watchdog(live,101,14999)
        != SipCommandDisposition::PendingTransition) return 4;
    if (executor.watchdog(live,101,15000)
        != SipCommandDisposition::CommandTimedOut) return 5;
    if (executor.watchdog(live,101,15001)
        != SipCommandDisposition::Fenced) return 6;
    if (executor.dispatch(dial(101,2),live,101,selected,native,15002).disposition
        != SipCommandDisposition::Fenced || native.outgoing != 1) return 7;

    live.session=102;
    if (executor.dispatch(dial(102,1),live,102,selected,native,16000).disposition
        != SipCommandDisposition::Submitted || native.outgoing != 2) return 8;
    live.sip=SipStatus::Calling;
    live.call_id=1;
    live.last_call_id=1;
    engine->state=SipStatus::Calling;
    if (executor.watchdog(live,102,16001)
        != SipCommandDisposition::ProgressObserved) return 9;
    if (executor.watchdog(live,102,26001)
        != SipCommandDisposition::Idle) return 10;
    SipCommandRequest hangup{102,2,CallAction::Hangup,1,{}};
    if (executor.dispatch(hangup,live,102,selected,native,26002).disposition
        != SipCommandDisposition::Submitted || native.hungup != 1) return 11;
    live.evidence=SipEvidence::None;
    if (executor.watchdog(live,102,26003)
        != SipCommandDisposition::StaleSession) return 12;
    live.evidence=SipEvidence::VerifiedNativeApi;
    if (executor.dispatch(hangup,live,102,selected,native,26004).disposition
        != SipCommandDisposition::Fenced || native.hungup != 1) return 13;

    live={RuntimeProcess::Running,SipStatus::Registered,
          SipEvidence::VerifiedNativeApi,103};
    engine->state=SipStatus::Registered;
    if (executor.dispatch(dial(103,1),live,103,selected,native,30000).disposition
        != SipCommandDisposition::Submitted || native.outgoing != 3) return 14;
    if (executor.watchdog(live,103,29999)
        != SipCommandDisposition::ClockRollback) return 15;
    if (executor.dispatch(dial(103,2),live,103,selected,native,30001).disposition
        != SipCommandDisposition::Fenced || native.outgoing != 3) return 16;

    // New generation must be able to recover without replay of the old one.
    live.session=104;
    if (executor.dispatch(dial(104,1),live,104,selected,native,31000).disposition
        != SipCommandDisposition::Submitted || native.outgoing != 4) return 17;
    if (executor.dispatch(dial(103,999),live,104,selected,native,31001).disposition
        != SipCommandDisposition::StaleSession || native.outgoing != 4) return 18;
    if (executor.dispatch(dial(104,2),live,104,selected,native,40999).disposition
        != SipCommandDisposition::PendingTransition) return 19;
    if (executor.dispatch(dial(104,3),live,104,selected,native,41000).disposition
        != SipCommandDisposition::CommandTimedOut) return 20;
    if (executor.dispatch(dial(104,4),live,104,selected,native,41001).disposition
        != SipCommandDisposition::Fenced || native.outgoing != 4) return 21;
    return 0;
}
