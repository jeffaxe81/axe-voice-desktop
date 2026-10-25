#include "../src/sip_command_executor.h"
#include <memory>
#include <stdexcept>

struct TestEngine final : axv::SipEngine {
    axv::SipStatus s = axv::SipStatus::Registered;
    axv::SipStatus status() const override {return s;}
    bool configure(const axv::SipAccount&) override {return false;}
    bool register_account() override {return false;}
    bool start_call(const std::wstring&) override {return false;}
    void hangup() override {}
    std::wstring diagnostic() const override {return L"synthetic";}
};
struct TestPort final : axv::SipNativeCommandPort {
    int dial=0,answer_count=0,hangups=0;
    bool fail=false, throws=false;
    std::uint64_t token=0;
    std::wstring target;
    bool originate(const std::wstring& dest) override {
        ++dial;target=dest;
        if(throws)throw std::runtime_error("synthetic");
        return !fail;
    }
    bool answer(std::uint64_t id) override {
        ++answer_count;token=id;
        if(throws)throw std::runtime_error("synthetic");
        return !fail;
    }
    bool hangup(std::uint64_t id) override {
        ++hangups;token=id;
        if(throws)throw std::runtime_error("synthetic");
        return !fail;
    }
};
int main() {
    using namespace axv;
    SipCommandExecutor executor;
    TestPort port;
    EngineSelection engine{std::make_unique<TestEngine>(),EngineKind::Baresip};
    auto* backend=static_cast<TestEngine*>(engine.engine.get());
    SipRuntimeObservation live{RuntimeProcess::Running,SipStatus::Registered,
        SipEvidence::VerifiedNativeApi,41};
    auto req=[](std::uint64_t session,std::uint64_t seq,CallAction action,
                std::uint64_t call=0,std::wstring destination={}) {
        return SipCommandRequest{session,seq,action,call,destination};
    };
    std::uint64_t now_ms=1000;
    auto send=[&](SipCommandRequest r) {
        now_ms+=10; // monotonic worker time; all normal transitions are prompt
        return executor.dispatch(r,live,live.session,engine,port,now_ms).disposition;
    };
    if(send(req(0,1,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::InvalidRequest) return 1;
    if(send(req(41,0,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::InvalidRequest) return 2;
    live.evidence=SipEvidence::None;
    if(send(req(41,1,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::StaleSession ||
       port.dial) return 3;
    live.evidence=SipEvidence::VerifiedNativeApi;
    if(send(req(41,1,CallAction::Dial,0,L"ana@example.org"))!=SipCommandDisposition::Submitted ||
       port.dial!=1 || port.target!=L"sip:ana@example.org") return 4;
    if(send(req(41,1,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::DuplicateRequest) return 5;
    if(send(req(41,2,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::PendingTransition ||
       port.dial!=1) return 6;
    live.sip=SipStatus::Calling;live.call_id=7;live.last_call_id=7;backend->s=SipStatus::Calling;
    if(send(req(41,3,CallAction::Hangup,6))!=SipCommandDisposition::Denied || port.hangups) return 7;
    if(send(req(41,4,CallAction::Hangup,7))!=SipCommandDisposition::Submitted ||
       port.hangups!=1 || port.token!=7) return 8;
    live.sip=SipStatus::InCall;backend->s=SipStatus::InCall;
    if(send(req(41,5,CallAction::Hangup,7))!=SipCommandDisposition::DuplicateCallAction ||
       port.hangups!=1) return 9;
    if(send(req(41,6,CallAction::Answer,7))!=SipCommandDisposition::Denied) return 10;
    live.sip=SipStatus::Registered;live.call_id=0;backend->s=SipStatus::Registered;
    if(send(req(41,7,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::Submitted ||
       port.dial!=2) return 11;
    if(send(req(41,8,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::PendingTransition) return 12;
    live.sip=SipStatus::Ringing;live.call_id=8;live.last_call_id=8;backend->s=SipStatus::Ringing;
    if(send(req(41,9,CallAction::Answer,8))!=SipCommandDisposition::Submitted ||
       port.answer_count!=1 || port.token!=8) return 13;
    live.sip=SipStatus::InCall;backend->s=SipStatus::InCall;
    if(send(req(41,10,CallAction::Answer,8))!=SipCommandDisposition::Denied) return 14;
    if(send(req(41,11,CallAction::Hangup,8))!=SipCommandDisposition::Submitted ||
       port.hangups!=2) return 15;
    live.sip=SipStatus::Registered;live.call_id=0;backend->s=SipStatus::Registered;
    if(send(req(41,12,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::Submitted ||
       port.dial!=3) return 16;
    live={RuntimeProcess::Running,SipStatus::Registered,SipEvidence::VerifiedNativeApi,42};
    if(send(req(42,1,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::Submitted ||
       port.dial!=4) return 17;
    if(send(req(41,1000,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::StaleSession ||
       port.dial!=4) return 18;
    live.sip=SipStatus::Failed;backend->s=SipStatus::Failed;
    if(send(req(42,2,CallAction::Hangup,8))!=SipCommandDisposition::Denied ||
       port.hangups!=2) return 19;
    live.sip=SipStatus::Registered;backend->s=SipStatus::Registered;
    live.evidence=SipEvidence::None;
    if(send(req(42,3,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::StaleSession) return 20;
    live.evidence=SipEvidence::VerifiedNativeApi;
    if(send(req(42,4,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::Fenced ||
       port.dial!=4) return 21;
    live={RuntimeProcess::Running,SipStatus::Registered,SipEvidence::VerifiedNativeApi,43};
    port.fail=true;
    if(send(req(43,1,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::NativeRejected ||
       port.dial!=5) return 22;
    port.fail=false;
    if(send(req(43,2,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::Fenced) return 23;
    live.session=44;port.throws=true;
    if(send(req(44,1,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::NativeRejected ||
       port.dial!=6) return 24;
    port.throws=false;
    if(send(req(44,2,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::Fenced) return 25;
    live.session=45;
    if(send(req(45,1,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::Submitted) return 26;
    const int count=port.dial;
    for(std::uint64_t i=2;i<102;++i) {
        if(send(req(45,i,CallAction::Dial,0,L"1001"))!=SipCommandDisposition::PendingTransition ||
           port.dial!=count) return 27;
    }
    EngineSelection unavailable=create_sip_engine();
    SipCommandExecutor isolated;
    if(isolated.dispatch(req(45,1,CallAction::Dial,0,L"1001"),
                         live,live.session,unavailable,port,now_ms+10).disposition !=
       SipCommandDisposition::Denied || port.dial!=count) return 28;
    return 0;
}
