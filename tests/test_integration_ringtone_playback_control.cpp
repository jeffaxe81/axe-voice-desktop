#include "../src/incoming_ringtone_coordinator.h"
#include "../src/baresip_event_mailbox.h"
#include "../src/sip_event_projection.h"
#include <vector>

struct FakeSink : axv::IncomingRingtoneSink {
    std::vector<char> calls;
    bool start(const std::wstring&) override { calls.push_back('S'); return true; }
    bool stop() override { calls.push_back('X'); return true; }
};
int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    SipEventProjection projection;
    FakeSink sink;
    IncomingRingtoneCoordinator coord;
    const auto session=mailbox.begin_session();
    mailbox.enqueue({BaresipEventKind::Registering,session});
    mailbox.enqueue({BaresipEventKind::RegisterOk,session});
    auto state=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,mailbox.drain());
    if(coord.update(state,session,L"pc",true,sink)!=IncomingRingtoneOutcome::Idle) return 1;
    mailbox.enqueue({BaresipEventKind::CallIncoming,session,1});
    state=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,mailbox.drain());
    if(coord.update(state,session,L"pc",true,sink)!=IncomingRingtoneOutcome::Started) return 2;
    if(coord.update(state,session,L"headset",true,sink)!=IncomingRingtoneOutcome::Stopped) return 3;
    if(coord.update(state,session,L"headset",true,sink)!=IncomingRingtoneOutcome::Started) return 4;
    mailbox.enqueue({BaresipEventKind::CallEstablished,session,1});
    state=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,mailbox.drain());
    if(coord.update(state,session,L"headset",true,sink)!=IncomingRingtoneOutcome::Stopped) return 5;
    mailbox.enqueue({BaresipEventKind::CallClosed,session,1});
    state=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,mailbox.drain());
    if(coord.update(state,session,L"headset",true,sink)!=IncomingRingtoneOutcome::Idle) return 6;
    mailbox.enqueue({BaresipEventKind::CallOutgoing,session,2});
    state=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,mailbox.drain());
    if(coord.update(state,session,L"pc",true,sink)!=IncomingRingtoneOutcome::Idle) return 7;
    mailbox.enqueue({BaresipEventKind::CallClosed,session,2});
    projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,mailbox.drain());
    mailbox.enqueue({BaresipEventKind::CallIncoming,session,3});
    state=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,mailbox.drain());
    if(coord.update(state,session,L"pc",true,sink)!=IncomingRingtoneOutcome::Started) return 8;
    // Invalid provider evidence immediately removes authorization.
    state=projection.observe(RuntimeProcess::Running,SipEvidence::None,mailbox.drain());
    if(coord.update(state,session,L"pc",true,sink)!=IncomingRingtoneOutcome::Stopped) return 9;
    // End-to-end actions: Start PC, Stop, Start headset, Stop, Start PC, Stop.
    const std::vector<char> expected{'S','X','S','X','S','X'};
    if(sink.calls!=expected || coord.playing() || coord.stop_pending()) return 10;
    return 0;
}
