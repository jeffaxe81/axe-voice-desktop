#include "../src/incoming_ringtone_gate.h"

int main() {
    using namespace axv;
    IncomingRingtoneGate ring;
    SipRuntimeObservation s{};
    if(ring.observe(s,0,true)!=IncomingRingAction::None) return 1;
    s={RuntimeProcess::Running,SipStatus::Ringing,SipEvidence::None,7,10,10};
    if(ring.observe(s,7,true)!=IncomingRingAction::None) return 2;
    s.evidence=SipEvidence::VerifiedNativeApi;
    if(ring.observe(s,8,true)!=IncomingRingAction::None) return 3;
    s.call_id=0;
    if(ring.observe(s,7,true)!=IncomingRingAction::None) return 4;
    s.call_id=11;
    if(ring.observe(s,7,true)!=IncomingRingAction::None) return 5;
    s.call_id=10;
    if(ring.observe(s,7,false)!=IncomingRingAction::None) return 6;
    if(ring.observe(s,7,true)!=IncomingRingAction::Start || !ring.active()) return 7;
    if(ring.observe(s,7,true)!=IncomingRingAction::None) return 8;
    if(ring.observe(s,7,false)!=IncomingRingAction::Stop || ring.active()) return 9;
    if(ring.observe(s,7,false)!=IncomingRingAction::None) return 10;
    if(ring.observe(s,7,true)!=IncomingRingAction::Start) return 11;
    s.sip=SipStatus::InCall;
    if(ring.observe(s,7,true)!=IncomingRingAction::Stop || ring.active()) return 12;
    s.sip=SipStatus::Registered;
    if(ring.observe(s,7,true)!=IncomingRingAction::None) return 13;
    s.sip=SipStatus::Calling;
    if(ring.observe(s,7,true)!=IncomingRingAction::None) return 14;
    s.sip=SipStatus::Ringing;
    if(ring.observe(s,7,true)!=IncomingRingAction::Start) return 15;
    s.session=8; s.last_call_id=1; s.call_id=1;
    if(ring.observe(s,8,true)!=IncomingRingAction::Stop) return 16;
    if(ring.observe(s,8,true)!=IncomingRingAction::Start) return 17;
    s.process=RuntimeProcess::Stopping;
    if(ring.observe(s,8,true)!=IncomingRingAction::Stop) return 18;
    s.process=RuntimeProcess::Running;
    if(ring.observe(s,8,true)!=IncomingRingAction::Start) return 19;
    s.evidence=SipEvidence::None;
    if(ring.observe(s,8,true)!=IncomingRingAction::Stop) return 20;
    s.evidence=SipEvidence::VerifiedNativeApi;
    if(ring.observe(s,8,true)!=IncomingRingAction::Start) return 21;
    if(ring.revoke()!=IncomingRingAction::Stop || ring.active()) return 22;
    if(ring.revoke()!=IncomingRingAction::None) return 23;
    return 0;
}
