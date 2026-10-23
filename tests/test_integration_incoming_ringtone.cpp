#include "../src/incoming_ringtone_gate.h"
#include "../src/baresip_event_mailbox.h"
#include "../src/sip_event_projection.h"

int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    SipEventProjection projection;
    IncomingRingtoneGate gate;

    // These are synthetic native events. They DO NOT mean real SIP was connected.
    const std::uint64_t session=mailbox.begin_session();
    if(!session || !mailbox.enqueue({BaresipEventKind::Registering,session}) ||
       !mailbox.enqueue({BaresipEventKind::RegisterOk,session})) return 1;
    auto s=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,
                              mailbox.drain());
    if(s.sip!=SipStatus::Registered ||
       gate.observe(s,session,true)!=IncomingRingAction::None) return 2;

    if(!mailbox.enqueue({BaresipEventKind::CallIncoming,session,1})) return 3;
    s=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,
                         mailbox.drain());
    if(s.sip!=SipStatus::Ringing ||
       gate.observe(s,session,true)!=IncomingRingAction::Start) return 4;
    if(!gate.active() || gate.observe(s,session,true)!=IncomingRingAction::None)
        return 5;

    if(!mailbox.enqueue({BaresipEventKind::CallEstablished,session,1})) return 6;
    s=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,
                         mailbox.drain());
    if(s.sip!=SipStatus::InCall ||
       gate.observe(s,session,true)!=IncomingRingAction::Stop) return 7;
    if(!mailbox.enqueue({BaresipEventKind::CallClosed,session,1})) return 8;
    s=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,
                         mailbox.drain());
    if(s.sip!=SipStatus::Registered ||
       gate.observe(s,session,true)!=IncomingRingAction::None) return 9;

    // Outgoing calls must never ring the incoming-call endpoint.
    if(!mailbox.enqueue({BaresipEventKind::CallOutgoing,session,2})) return 10;
    s=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,
                         mailbox.drain());
    if(s.sip!=SipStatus::Calling ||
       gate.observe(s,session,true)!=IncomingRingAction::None) return 11;
    if(!mailbox.enqueue({BaresipEventKind::CallClosed,session,2})) return 12;
    s=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,
                         mailbox.drain());

    if(!mailbox.enqueue({BaresipEventKind::CallIncoming,session,3})) return 13;
    s=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,
                         mailbox.drain());
    if(gate.observe(s,session,true)!=IncomingRingAction::Start) return 14;
    if(!mailbox.enqueue({BaresipEventKind::TransportLost,session})) return 15;
    s=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,
                         mailbox.drain());
    if(s.sip!=SipStatus::Disconnected ||
       gate.observe(s,session,true)!=IncomingRingAction::Stop) return 16;

    // Same token in a new epoch: only newly verified session can start.
    mailbox.invalidate_session();
    const auto new_session=mailbox.begin_session();
    if(new_session<=session) return 17;
    if(!mailbox.enqueue({BaresipEventKind::Registering,new_session}) ||
       !mailbox.enqueue({BaresipEventKind::RegisterOk,new_session}) ||
       !mailbox.enqueue({BaresipEventKind::CallIncoming,new_session,1})) return 18;
    s=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,
                         mailbox.drain());
    if(gate.observe(s,session,true)!=IncomingRingAction::None) return 19;
    if(gate.observe(s,new_session,true)!=IncomingRingAction::Start) return 20;
    // One overflow must cancel and reject every event from the current epoch.
    for(std::size_t i=0;i<BaresipEventMailbox::kCapacity;++i)
        if(!mailbox.enqueue({BaresipEventKind::RegisterOk,new_session}))
            return 21;
    if(mailbox.enqueue({BaresipEventKind::RegisterOk,new_session})) return 22;
    s=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,
                         mailbox.drain());
    if(gate.observe(s,new_session,true)!=IncomingRingAction::Stop) return 23;
    if(gate.observe(s,new_session,true)!=IncomingRingAction::None) return 24;
    return 0;
}
