#include "../src/baresip_event_identity.h"
int main() {
    using namespace axv;
    BoundBaresipState s;
    s.observation = {RuntimeProcess::Running,SipStatus::Disconnected,SipEvidence::None,9};
    s.expected_account_token = 40;
    const auto send = [&](BaresipEventKind kind, std::uint64_t call=0,
                          std::uint64_t account=40, std::uint64_t session=9) {
        return apply_identified_event(s,9,{kind,session,0,account,call});
    };
    if(send(BaresipEventKind::Registering)!=IdentityResult::UnverifiedSource) return 1;
    s.observation.evidence=SipEvidence::VerifiedNativeApi; // Synthetic policy test only.
    if(send(BaresipEventKind::Registering,0,41)!=IdentityResult::WrongAccount) return 2;
    if(send(BaresipEventKind::Registering,0,40,8)!=IdentityResult::StaleSession) return 3;
    if(send(BaresipEventKind::Registering,33)!=IdentityResult::UnexpectedCall) return 4;
    if(send(BaresipEventKind::RegisterOk)!=IdentityResult::InvalidOrder) return 5;
    if(send(BaresipEventKind::Registering)!=IdentityResult::Applied) return 6;
    if(send(BaresipEventKind::RegisterOk)!=IdentityResult::Applied) return 7;
    if(send(BaresipEventKind::CallIncoming,0)!=IdentityResult::MissingCall) return 8;
    if(send(BaresipEventKind::CallIncoming,101,41)!=IdentityResult::WrongAccount) return 9;
    if(send(BaresipEventKind::CallIncoming,101)!=IdentityResult::Applied) return 10;
    if(s.observation.sip!=SipStatus::Ringing || s.active_call_token!=101) return 11;
    if(send(BaresipEventKind::CallIncoming,102)!=IdentityResult::ConcurrentCall) return 12;
    if(send(BaresipEventKind::CallEstablished,102)!=IdentityResult::WrongCall) return 13;
    if(send(BaresipEventKind::CallEstablished,101)!=IdentityResult::Applied) return 14;
    if(send(BaresipEventKind::CallClosed,102)!=IdentityResult::WrongCall) return 15;
    if(send(BaresipEventKind::CallClosed,101)!=IdentityResult::Applied) return 16;
    if(s.active_call_token) return 17;
    if(send(BaresipEventKind::CallIncoming,101)!=IdentityResult::ReplayedCall) return 18;
    if(send(BaresipEventKind::CallOutgoing,102)!=IdentityResult::Applied) return 19;
    if(send(BaresipEventKind::TransportLost,102)!=IdentityResult::UnexpectedCall) return 20;
    if(send(BaresipEventKind::TransportLost)!=IdentityResult::Applied) return 21;
    if(s.active_call_token || s.observation.sip!=SipStatus::Disconnected) return 22;
    if(send(BaresipEventKind::CallClosed,102)!=IdentityResult::WrongCall) return 23;
    if(send(BaresipEventKind::Registering)!=IdentityResult::Applied) return 24;
    if(send(BaresipEventKind::RegisterOk)!=IdentityResult::Applied) return 25;
    if(send(BaresipEventKind::CallOutgoing,102)!=IdentityResult::ReplayedCall) return 26;
    if(send(BaresipEventKind::CallOutgoing,103)!=IdentityResult::Applied) return 27;
    s.observation.process=RuntimeProcess::Stopping;
    if(send(BaresipEventKind::CallEstablished,103)!=IdentityResult::UnverifiedSource) return 28;
    invalidate_identity(s);
    if(s.expected_account_token || s.active_call_token ||
       s.observation.evidence!=SipEvidence::None) return 29;
    s.observation={RuntimeProcess::Running,SipStatus::Disconnected,SipEvidence::VerifiedNativeApi,10};
    s.expected_account_token=41;
    if(apply_identified_event(s,10,{BaresipEventKind::Registering,9,0,41,0})!=IdentityResult::StaleSession) return 30;
    if(apply_identified_event(s,10,{BaresipEventKind::Registering,10,0,40,0})!=IdentityResult::WrongAccount) return 31;
    return 0;
}
