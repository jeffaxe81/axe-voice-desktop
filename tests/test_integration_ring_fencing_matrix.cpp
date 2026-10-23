#include "../src/incoming_ringtone_coordinator.h"
#include <cstdint>
#include <string>

struct CheckedSink final : axv::IncomingRingtoneSink {
    bool active=false;
    bool deny_stop=false;
    bool overlap=false;
    unsigned starts=0, stops=0;
    bool start(const std::wstring& id) override {
        if(active || id.empty()) overlap=true;
        active=true;
        ++starts;
        return true;
    }
    bool stop() override {
        ++stops;
        if(deny_stop) return false;
        active=false;
        return true;
    }
};
static std::uint32_t next(std::uint32_t& n) {
    n ^= n << 13;
    n ^= n >> 17;
    n ^= n << 5;
    return n;
}
int main() {
    using namespace axv;
    IncomingRingtoneCoordinator ring;
    CheckedSink sink;
    std::uint32_t seed=0x9e3779b9u;
    constexpr unsigned cases=20000;
    for(unsigned i=0;i<cases;++i) {
        const auto r=next(seed);
        SipRuntimeObservation s{};
        s.process=(r&1)?RuntimeProcess::Running:RuntimeProcess::Stopping;
        s.evidence=(r&2)?SipEvidence::VerifiedNativeApi:SipEvidence::None;
        switch((r>>2)%5) {
            case 0: s.sip=SipStatus::Ringing; break;
            case 1: s.sip=SipStatus::InCall; break;
            case 2: s.sip=SipStatus::Calling; break;
            case 3: s.sip=SipStatus::Registered; break;
            default: s.sip=SipStatus::Disconnected; break;
        }
        s.session=((r>>5)&1)?7:8;
        s.call_id=1+((r>>6)%13);
        s.last_call_id=((r>>10)&3)?s.call_id:s.call_id+1;
        const auto proof=((r>>14)&1)?s.session:9;
        const auto outputCode=(r>>15)&3;
        const std::wstring endpoint=outputCode==0?L"":
            outputCode==1?L"PC":L"HEADSET";
        const bool available=((r>>17)&3)!=0;
        sink.deny_stop=((r>>19)&7)==0;
        const auto startsBefore=sink.starts;
        const auto out=ring.update(s,proof,endpoint,available,sink);
        if(sink.overlap) return 1;
        if(ring.playing() && !sink.active) return 2;
        if(ring.stop_pending() && !sink.active) return 3;
        if(out==IncomingRingtoneOutcome::Started) {
            if(sink.starts!=startsBefore+1 || !ring.playing()) return 4;
            if(s.process!=RuntimeProcess::Running ||
               s.evidence!=SipEvidence::VerifiedNativeApi ||
               s.sip!=SipStatus::Ringing || s.session!=proof ||
               s.call_id!=s.last_call_id || !available || endpoint.empty()) return 5;
        }
        if(out!=IncomingRingtoneOutcome::Started && sink.starts!=startsBefore)
            return 6;
    }
    sink.deny_stop=false;
    if(ring.revoke(sink)==IncomingRingtoneOutcome::StopPending ||
       ring.playing() || ring.stop_pending() || sink.active || sink.overlap)
        return 7;
    if(sink.starts==0 || sink.stops==0) return 8;
    // Without native proof, no Start is admissible, regardless of endpoint.
    for(int i=0;i<1000;++i) {
        SipRuntimeObservation noProof{RuntimeProcess::Running,SipStatus::Ringing,
            SipEvidence::None,7,1,1};
        if(ring.update(noProof,7,L"HEADSET",true,sink)==IncomingRingtoneOutcome::Started)
            return 9;
    }
    if(sink.active || sink.overlap) return 10;
    return 0;
}
