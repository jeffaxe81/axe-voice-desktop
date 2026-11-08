#include "../src/native_session_control.h"
#include <cstdlib>
#define CHECK(x) do {if(!(x))std::abort();}while(0)
int main(){
 axv::SipEventProjection projection;
 axv::BaresipEventMailbox::Batch b{};b.session=1;b.size=2;
 b.events[0]={axv::BaresipEventKind::Registering,1,0};b.events[1]={axv::BaresipEventKind::RegisterOk,1,0};
 auto s=axv::project_native_sdk_batch(projection,b,true);CHECK(s.sip==axv::SipStatus::Registered);
 b.size=1;b.events[0]={axv::BaresipEventKind::RegisterOk,1,0};
 s=axv::project_native_sdk_batch(projection,b,true);CHECK(s.sip==axv::SipStatus::Registered && s.session==1);
 b.events[0]={axv::BaresipEventKind::CallIncoming,1,1};s=axv::project_native_sdk_batch(projection,b,true);
 auto answer=axv::NativeCallCommand{axv::NativeCallAction::Answer,1,1,{}};
 CHECK(axv::native_command_is_current(answer,s));
 axv::NativeCommandAdmission admission;
 CHECK(admission.admit(answer,s));CHECK(!admission.admit(answer,s));
 auto next_answer=answer;next_answer.call_id=2;
 auto replacement=s;replacement.call_id=2;
 CHECK(admission.admit(next_answer,replacement));CHECK(!admission.admit(answer,replacement));
 auto next=s;next.call_id=2;CHECK(!axv::native_command_is_current(answer,next));
 next=s;next.session=2;CHECK(!axv::native_command_is_current(answer,next));
 auto hangup=axv::NativeCallCommand{axv::NativeCallAction::Hangup,1,1,{}};
 CHECK(admission.admit(hangup,s));CHECK(!admission.admit(hangup,s));
 CHECK(axv::native_command_is_current(hangup,s));next=s;next.call_id=2;CHECK(!axv::native_command_is_current(hangup,next));
 b.events[0]={axv::BaresipEventKind::CallEstablished,1,1};s=axv::project_native_sdk_batch(projection,b,true);CHECK(s.sip==axv::SipStatus::InCall);
 b.events[0]={axv::BaresipEventKind::RegisterOk,1,0};s=axv::project_native_sdk_batch(projection,b,true);CHECK(s.sip==axv::SipStatus::InCall && s.call_id==1);
 s=axv::project_native_sdk_batch(projection,b,false);CHECK(s.evidence==axv::SipEvidence::None);
 axv::SipEventProjection lost;
 b.size=2;b.events[0]={axv::BaresipEventKind::Registering,1,0};b.events[1]={axv::BaresipEventKind::RegisterOk,1,0};
 s=axv::project_native_sdk_batch(lost,b,true);CHECK(s.sip==axv::SipStatus::Registered);
 b.size=0;s=axv::project_native_sdk_batch(lost,b,false);CHECK(s.evidence==axv::SipEvidence::None);
 // Two thousand fully synthetic call lifecycles exercise the Baresip event
 // projection and native command guards. These are NOT real PBX/RTP calls.
 for (std::uint64_t iteration=1;iteration<=1000;++iteration) {
  axv::SipEventProjection receiver;
  axv::BaresipEventMailbox::Batch incoming{};
  incoming.session=iteration+100;
  incoming.size=2;
  incoming.events[0]={axv::BaresipEventKind::Registering,incoming.session,0};
  incoming.events[1]={axv::BaresipEventKind::RegisterOk,incoming.session,0};
  auto state=axv::project_native_sdk_batch(receiver,incoming,true);
  CHECK(state.sip==axv::SipStatus::Registered && state.call_id==0);
  incoming.size=1;
  incoming.events[0]={axv::BaresipEventKind::CallIncoming,incoming.session,1};
  state=axv::project_native_sdk_batch(receiver,incoming,true);
  CHECK(state.sip==axv::SipStatus::Ringing && state.call_id==1);
  const axv::NativeCallCommand accept{axv::NativeCallAction::Answer,incoming.session,1,{}};
  axv::NativeCommandAdmission only_once;
  CHECK(axv::native_command_is_current(accept,state));
  CHECK(only_once.admit(accept,state) && !only_once.admit(accept,state));
  incoming.events[0]={axv::BaresipEventKind::CallEstablished,incoming.session,1};
  state=axv::project_native_sdk_batch(receiver,incoming,true);
  CHECK(state.sip==axv::SipStatus::InCall && !axv::native_command_is_current(accept,state));
  const axv::NativeCallCommand end{axv::NativeCallAction::Hangup,incoming.session,1,{}};
  CHECK(only_once.admit(end,state) && !only_once.admit(end,state));
  incoming.events[0]={axv::BaresipEventKind::CallClosed,incoming.session,1};
  state=axv::project_native_sdk_batch(receiver,incoming,true);
  CHECK(state.sip==axv::SipStatus::Registered && state.call_id==0);
  CHECK(!axv::native_command_is_current(end,state));
  // Reusing a closed call token must revoke proof, never show a new ring.
  incoming.events[0]={axv::BaresipEventKind::CallIncoming,incoming.session,1};
  state=axv::project_native_sdk_batch(receiver,incoming,true);
  CHECK(state.evidence==axv::SipEvidence::None && state.sip!=axv::SipStatus::Ringing);

  axv::SipEventProjection originator;
  axv::BaresipEventMailbox::Batch outgoing{};
  outgoing.session=iteration+1100;
  outgoing.size=2;
  outgoing.events[0]={axv::BaresipEventKind::Registering,outgoing.session,0};
  outgoing.events[1]={axv::BaresipEventKind::RegisterOk,outgoing.session,0};
  state=axv::project_native_sdk_batch(originator,outgoing,true);
  CHECK(state.sip==axv::SipStatus::Registered);
  outgoing.size=1;
  outgoing.events[0]={axv::BaresipEventKind::CallOutgoing,outgoing.session,1};
  state=axv::project_native_sdk_batch(originator,outgoing,true);
  CHECK(state.sip==axv::SipStatus::Calling && state.call_id==1);
  const axv::NativeCallCommand cancel{axv::NativeCallAction::Hangup,outgoing.session,1,{}};
  CHECK(axv::native_command_is_current(cancel,state));
  outgoing.events[0]={axv::BaresipEventKind::CallClosed,outgoing.session,1};
  state=axv::project_native_sdk_batch(originator,outgoing,true);
  CHECK(state.sip==axv::SipStatus::Registered && !axv::native_command_is_current(cancel,state));
 }

 // Native call commands are authorized against the live session, not the
 // displayed text. Exhaust process/evidence/token changes for 1,000
 // synthetic session/call pairs; these do NOT make real RTP/SIP calls.
 for(std::uint64_t iteration=1;iteration<=1000;++iteration) {
  const std::uint64_t session=iteration+5000, call=iteration+7000;
  const axv::SipRuntimeObservation ringing{
     axv::RuntimeProcess::Running,axv::SipStatus::Ringing,
     axv::SipEvidence::VerifiedNativeApi,session,call,call-1};
  const axv::SipRuntimeObservation calling{
     axv::RuntimeProcess::Running,axv::SipStatus::Calling,
     axv::SipEvidence::VerifiedNativeApi,session,call,call-1};
  const axv::SipRuntimeObservation in_call{
     axv::RuntimeProcess::Running,axv::SipStatus::InCall,
     axv::SipEvidence::VerifiedNativeApi,session,call,call-1};
  const axv::SipRuntimeObservation registered{
     axv::RuntimeProcess::Running,axv::SipStatus::Registered,
     axv::SipEvidence::VerifiedNativeApi,session,0,call-1};
  const axv::NativeCallCommand answer{axv::NativeCallAction::Answer,session,call,{}};
  const axv::NativeCallCommand end{axv::NativeCallAction::Hangup,session,call,{}};
  const axv::NativeCallCommand mute{axv::NativeCallAction::SetMute,session,call,{}};
  const axv::NativeCallCommand hold{axv::NativeCallAction::SetHold,session,call,{}};
  const axv::NativeCallCommand transfer{axv::NativeCallAction::Transfer,session,call,{}};
  const axv::NativeCallCommand dtmf{axv::NativeCallAction::Dtmf,session,call,"5"};
  const axv::NativeCallCommand dial{axv::NativeCallAction::Dial,session,0,"1002"};
  CHECK(axv::native_command_is_current(dial,registered));
  CHECK(!axv::native_command_is_current(dial,ringing));
  CHECK(axv::native_command_is_current(answer,ringing));
  CHECK(!axv::native_command_is_current(answer,in_call));
  CHECK(axv::native_command_is_current(end,ringing));
  CHECK(axv::native_command_is_current(end,calling));
  CHECK(axv::native_command_is_current(end,in_call));
  for(const auto& c:{mute,hold,transfer,dtmf}) {
   CHECK(axv::native_command_is_current(c,in_call));
   CHECK(!axv::native_command_is_current(c,ringing));
   CHECK(!axv::native_command_is_current(c,calling));
  }
  CHECK(axv::native_displayed_call_command_current(axv::NativeCallAction::Answer,ringing,ringing));
  CHECK(!axv::native_displayed_call_command_current(axv::NativeCallAction::SetMute,in_call,in_call));
  auto stale=ringing;
  stale.call_id=call+1;
  CHECK(!axv::native_displayed_call_command_current(axv::NativeCallAction::Answer,ringing,stale));
  stale=ringing;stale.session=session+1;
  CHECK(!axv::native_displayed_call_command_current(axv::NativeCallAction::Answer,ringing,stale));
  stale=ringing;stale.evidence=axv::SipEvidence::None;
  CHECK(!axv::native_command_is_current(end,stale));
  stale=ringing;stale.process=axv::RuntimeProcess::Failed;
  CHECK(!axv::native_command_is_current(answer,stale));
  axv::NativeCommandAdmission once;
  CHECK(once.admit(answer,ringing) && !once.admit(answer,ringing));
  CHECK(once.admit(transfer,in_call) && !once.admit(transfer,in_call));
  CHECK(once.admit(end,in_call) && !once.admit(end,in_call));
  CHECK(once.admit(dtmf,in_call) && once.admit(dtmf,in_call));
 }

}
