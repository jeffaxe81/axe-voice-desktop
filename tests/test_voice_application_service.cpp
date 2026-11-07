#include "../src/voice_application_service.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <utility>

namespace {
using axv::RuntimeProcess;
using axv::SipEvidence;
using axv::SipRuntimeObservation;
using axv::SipStatus;

void check(bool value,const char* reason){
    if(!value){std::fprintf(stderr,"FAIL %s\n",reason);std::abort();}
}
SipRuntimeObservation current(SipStatus status,std::uint64_t session,
                              std::uint64_t call_id,bool proven=true) {
    return {RuntimeProcess::Running,status,
            proven?SipEvidence::VerifiedNativeApi:SipEvidence::None,
            session,call_id,call_id};
}
struct FakeNativePort {
    SipRuntimeObservation live;
    bool accepted=true;
    int dials=0,answers=0,hangups=0;
    std::wstring submitted;
    SipRuntimeObservation observation()const{return live;}
    bool start_call(const std::wstring& destination){
        ++dials;submitted=destination;return accepted;
    }
    bool answer_call(const SipRuntimeObservation& displayed){
        ++answers;
        return accepted && axv::native_displayed_call_command_current(
            axv::NativeCallAction::Answer,displayed,live);
    }
    bool hangup_call(const SipRuntimeObservation& displayed){
        ++hangups;
        return accepted && axv::native_displayed_call_command_current(
            axv::NativeCallAction::Hangup,displayed,live);
    }
};
}
int main(){
    FakeNativePort port{};
    axv::VoiceApplicationService<FakeNativePort> service(port);
    auto screen=service.snapshot(true);
    check(screen.observation.process==RuntimeProcess::Stopped,"initial stop");
    check(screen.actions.view==axv::EssentialCallView::Idle &&
          !screen.actions.dial && screen.signal==axv::EssentialConnectionSignal::Offline,
          "never pretend registration");
    const auto idle=current(SipStatus::Registered,7,0);
    port.live=idle;
    screen=service.snapshot(true);
    check(screen.signal==axv::EssentialConnectionSignal::Online &&
          screen.actions.dial && !screen.actions.answer,"verified dial visibility");
    check(!service.dial(idle,L"https://invalid.example") &&
          !service.dial(idle,L" 1001"),"destination rejected");
    check(port.dials==0,"bad destination reached native engine");
    check(service.dial(idle,L"1001") && port.dials==1 &&
          port.submitted==L"1001","valid dial delegated");
    port.accepted=false;
    check(!service.dial(idle,L"1002") && port.dials==2,
          "native rejection must propagate");
    port.accepted=true;

    const auto incoming=current(SipStatus::Ringing,7,101);
    port.live=incoming;
    check(service.answer(incoming) && port.answers==1,"verified incoming accepted");
    check(!service.dial(idle,L"1001") && port.dials==2,"dial blocked during ringing");
    check(service.hangup(incoming) && port.hangups==1,"verified incoming refused");

    const auto calling=current(SipStatus::Calling,7,101);
    port.live=calling;
    check(!service.answer(calling) && port.answers==1,"cannot answer outgoing");
    check(service.hangup(calling) && port.hangups==2,"outgoing can hangup");
    const auto connected=current(SipStatus::InCall,7,101);
    port.live=connected;
    check(!service.answer(connected),"cannot answer connected");
    check(service.hangup(connected),"can hangup connected");

    const auto different=current(SipStatus::Ringing,8,102);
    port.live=different;
    check(!service.answer(incoming) && !service.hangup(incoming),
          "previous session cannot operate a new call");
    const auto same_session_new_call=current(SipStatus::Ringing,7,102);
    port.live=same_session_new_call;
    check(!service.answer(incoming) && !service.hangup(incoming),
          "same session different call cannot operate");
    port.live=current(SipStatus::Ringing,7,0);
    check(!service.answer(port.live),"zero call id must deny");
    port.live=current(SipStatus::Ringing,7,101,false);
    check(!service.answer(incoming) && !service.hangup(incoming),
          "unverified native state denied");
    port.live=current(SipStatus::Registering,7,0);
    check(!service.dial(idle,L"1001"),"registering cannot originate");

    // Exercise a high-volume stale/current session matrix without SIP I/O.
    const int old_answers=port.answers,old_hangups=port.hangups;
    for(std::uint64_t i=1;i<=2000;++i){
        const auto verified=current(SipStatus::Ringing,i,i);
        port.live=verified;
        check(service.answer(verified),"verified batch incoming");
        port.live=current(SipStatus::Ringing,i+1,i);
        check(!service.answer(verified) && !service.hangup(verified),
              "stale batch session must not reach native");
    }
    check(port.answers==old_answers+2000 && port.hangups==old_hangups,
          "stale call improperly reached native");
    std::puts("VoiceApplicationService: 2000 current + 2000 stale sessions and command guards passed");
}
