#include "../src/incoming_ringtone_coordinator.h"
#include <string>
#include <vector>

class FakeSink final : public axv::IncomingRingtoneSink {
public:
    bool allow_start = true;
    bool allow_stop = true;
    int starts = 0;
    int stops = 0;
    std::vector<std::wstring> outputs;
    bool start(const std::wstring& output) override {
        ++starts; outputs.push_back(output);
        return allow_start;
    }
    bool stop() override { ++stops; return allow_stop; }
};
int main() {
    using namespace axv;
    FakeSink sink;
    IncomingRingtoneCoordinator routing;
    SipRuntimeObservation call{RuntimeProcess::Running, SipStatus::Ringing,
        SipEvidence::VerifiedNativeApi, 7, 11, 11};
    auto update = [&](const std::wstring& output, bool available=true) {
        return routing.update(call,7,output,available,sink);
    };

    if(update(L"headset")!=IncomingRingtoneOutcome::Started ||
       sink.starts!=1 || sink.outputs[0]!=L"headset" || !routing.playing()) return 1;
    if(update(L"headset")!=IncomingRingtoneOutcome::Idle || sink.starts!=1) return 2;
    // The output cannot switch until the old renderer positively acknowledges stop.
    sink.allow_stop=false;
    if(update(L"pc")!=IncomingRingtoneOutcome::StopPending ||
       !routing.stop_pending() || sink.stops!=1 || sink.starts!=1) return 3;
    if(update(L"pc")!=IncomingRingtoneOutcome::StopPending ||
       sink.stops!=2 || sink.starts!=1) return 4;
    sink.allow_stop=true;
    if(update(L"pc")!=IncomingRingtoneOutcome::Stopped ||
       sink.stops!=3 || sink.starts!=1) return 5;
    if(update(L"pc")!=IncomingRingtoneOutcome::Started ||
       sink.starts!=2 || sink.outputs[1]!=L"pc") return 6;
    call.sip=SipStatus::InCall;
    if(update(L"pc")!=IncomingRingtoneOutcome::Stopped ||
       routing.playing()) return 7;
    call.sip=SipStatus::Registered;
    if(update(L"pc")!=IncomingRingtoneOutcome::Idle) return 8;
    call.sip=SipStatus::Ringing;call.call_id=12;call.last_call_id=12;
    if(update(L"pc")!=IncomingRingtoneOutcome::Started) return 9;
    // A revoked native session must silence the active output.
    call.evidence=SipEvidence::None;
    if(update(L"pc")!=IncomingRingtoneOutcome::Stopped) return 10;
    if(update(L"pc")!=IncomingRingtoneOutcome::Idle) return 11;
    call.evidence=SipEvidence::VerifiedNativeApi;
    // Unplug must force cancellation. No silent failover to unknown output.
    if(update(L"pc")!=IncomingRingtoneOutcome::Started) return 12;
    if(update(L"pc",false)!=IncomingRingtoneOutcome::Stopped) return 13;

    // A failed start may partially activate hardware; require explicit stop.
    FakeSink bad;
    bad.allow_start=false; bad.allow_stop=false;
    IncomingRingtoneCoordinator another;
    const auto first=another.update(call,7,L"headset",true,bad);
    if(first!=IncomingRingtoneOutcome::StopPending ||
       bad.starts!=1 || bad.stops!=1 || !another.stop_pending()) return 14;
    bad.allow_stop=true;
    if(another.update(call,7,L"headset",true,bad)!=IncomingRingtoneOutcome::Stopped)
        return 15;
    if(another.update(call,7,L"headset",true,bad)!=IncomingRingtoneOutcome::Idle ||
       bad.starts!=1) return 16; // No infinite reattempt storm.
    call.call_id=13; call.last_call_id=13;
    if(another.update(call,7,L"headset",true,bad)!=IncomingRingtoneOutcome::StopPending ||
       bad.starts!=2) return 17; // Another attempted Start, still fails.
    if(another.revoke(bad)!=IncomingRingtoneOutcome::Idle &&
       another.stop_pending()) return 18;

    // Closing a window must await stop ACK if a sink refuses to stop.
    FakeSink shutdown;
    IncomingRingtoneCoordinator end;
    if(end.update(call,7,L"pc",true,shutdown)!=IncomingRingtoneOutcome::Started) return 19;
    shutdown.allow_stop=false;
    if(end.revoke(shutdown)!=IncomingRingtoneOutcome::StopPending ||
       !end.stop_pending()) return 20;
    shutdown.allow_stop=true;
    if(end.revoke(shutdown)!=IncomingRingtoneOutcome::Stopped ||
       end.stop_pending()) return 21;

    // Disabled production sink never emits anything (it is not a player).
    DisabledIncomingRingtoneSink disabled;
    IncomingRingtoneCoordinator noop;
    if(noop.update(call,7,L"pc",true,disabled)!=IncomingRingtoneOutcome::StartRejected ||
       noop.playing()) return 22;
    if(noop.update(call,7,L"pc",true,disabled)!=IncomingRingtoneOutcome::Idle)
        return 23;
    return 0;
}
