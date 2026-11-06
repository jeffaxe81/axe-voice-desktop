// 2,000 deterministic SIP call-lifecycle simulations (not real network calls).
// 1,000 inbound + 1,000 outbound; all events are synthetic and verified only
// within the state reducer. No real SIP INVITE, RTP, registrar or PABX involved.
#include "../src/baresip_event_reducer.h"
#include <cstdint>
#include <iostream>

int main() {
    using namespace axv;
    int inbound = 0, outbound = 0, failures = 0;
    for (int index = 0; index < 2000; ++index) {
        const bool incoming = index < 1000;
        const auto session = static_cast<std::uint64_t>(index + 1);
        const auto call = static_cast<std::uint64_t>(index + 100);
        SipRuntimeObservation state{RuntimeProcess::Running, SipStatus::Registered,
                                    SipEvidence::VerifiedNativeApi, session};
        const auto send = [&](BaresipEventKind kind, std::uint64_t event_session,
                              std::uint64_t id) {
            return apply_baresip_event(state, session, {kind,event_session,id});
        };
        const auto begin = incoming ? BaresipEventKind::CallIncoming
                                    : BaresipEventKind::CallOutgoing;
        const auto expected = incoming ? SipStatus::Ringing : SipStatus::Calling;
        bool good = send(begin,session,call) == EventTransition::Applied &&
                    state.sip == expected && state.call_id == call;
        // Every test checks wrong-session callback and duplicate call rejection.
        good = good && send(BaresipEventKind::CallEstablished,session+1,call)
                     == EventTransition::StaleSession;
        good = good && send(begin,session,call) == EventTransition::InvalidOrder;
        if (index % 4 == 0) {
            // Normal connected call, followed by a stale BYE.
            good = good && send(BaresipEventKind::CallEstablished,session,call)
                         == EventTransition::Applied;
            good = good && state.sip == SipStatus::InCall;
            good = good && send(BaresipEventKind::CallClosed,session,call)
                         == EventTransition::Applied;
            good = good && send(BaresipEventKind::CallClosed,session,call)
                         == EventTransition::InvalidOrder;
            good = good && state.sip == SipStatus::Registered &&
                           state.call_id == 0;
        } else if (index % 4 == 1) {
            // Caller hangs up while still ringing or dialing.
            good = good && send(BaresipEventKind::CallClosed,session,call)
                         == EventTransition::Applied;
            good = good && state.sip == SipStatus::Registered &&
                           state.call_id == 0;
        } else if (index % 4 == 2) {
            // Network failure during setup prevents further call control.
            good = good && send(BaresipEventKind::TransportLost,session,0)
                         == EventTransition::Applied;
            good = good && state.sip == SipStatus::Disconnected &&
                           state.call_id == 0;
            good = good && !may_use_call_control(
                state,session,incoming ? CallAction::Answer : CallAction::Dial);
        } else {
            // Reject a forged old call token, then recover cleanly.
            good = good && send(BaresipEventKind::CallEstablished,session,call-1)
                         == EventTransition::StaleCall;
            good = good && state.sip == expected && state.call_id == call;
            good = good && send(BaresipEventKind::CallClosed,session,call)
                         == EventTransition::Applied;
            good = good && send(begin,session,call) == EventTransition::StaleCall;
        }
        if (good) {
            if (incoming) ++inbound; else ++outbound;
        } else {
            ++failures;
            std::cerr << "FAILED lifecycle " << (index+1)
                      << (incoming ? " inbound" : " outbound") << '\n';
        }
    }
    std::cout << "SIP synthetic call lifecycle: received_passed=" << inbound
              << "/1000 originated_passed=" << outbound
              << "/1000 failed=" << failures << '\n';
    return inbound == 1000 && outbound == 1000 && failures == 0 ? 0 : 1;
}
