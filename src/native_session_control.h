#pragma once
#include "sip_event_projection.h"
#include <string>
namespace axv {
enum class NativeCallAction {None,Dial,Answer,Hangup,SetMute,SetHold,Transfer};
struct NativeCallCommand {
 NativeCallAction action=NativeCallAction::None;
 std::uint64_t session=0,call_id=0;
 std::string destination;
 bool desired=false;
};
inline bool native_command_is_current(const NativeCallCommand& c,const SipRuntimeObservation& s){
 if(!sip_proof_is_current(s,c.session) || c.call_id!=s.call_id)return false;
 switch(c.action){
 case NativeCallAction::Dial:return s.sip==SipStatus::Registered && !s.call_id;
 case NativeCallAction::Answer:return s.sip==SipStatus::Ringing && s.call_id;
 case NativeCallAction::Transfer:
 case NativeCallAction::SetMute:
 case NativeCallAction::SetHold:return s.sip==SipStatus::InCall && s.call_id;
 case NativeCallAction::Hangup:return s.call_id && (s.sip==SipStatus::Calling||s.sip==SipStatus::Ringing||s.sip==SipStatus::InCall);
 default:return false;
 }
}
inline bool native_displayed_call_command_current(NativeCallAction action,const SipRuntimeObservation& displayed,const SipRuntimeObservation& current){
 if(action!=NativeCallAction::Answer && action!=NativeCallAction::Hangup)return false;
 const NativeCallCommand command{action,displayed.session,displayed.call_id,{}};
 return native_command_is_current(command,displayed) && native_command_is_current(command,current);
}
// UI double-clicks must not turn a declined duplicate into call teardown.
class NativeCommandAdmission {
 std::uint64_t answer_session_=0,answer_call_=0,hangup_session_=0,hangup_call_=0,transfer_session_=0,transfer_call_=0;
public:
 bool admit(const NativeCallCommand& c,const SipRuntimeObservation& s){
  if(!native_command_is_current(c,s))return false;
  if(c.action==NativeCallAction::Answer){
   if(answer_session_==c.session && answer_call_==c.call_id)return false;
   answer_session_=c.session;answer_call_=c.call_id;
  }
  if(c.action==NativeCallAction::Hangup){
   if(hangup_session_==c.session && hangup_call_==c.call_id)return false;
   hangup_session_=c.session;hangup_call_=c.call_id;
  }
  if(c.action==NativeCallAction::Transfer){
   if(transfer_session_==c.session && transfer_call_==c.call_id)return false;
   transfer_session_=c.session;transfer_call_=c.call_id;
  }
  return true;
 }
};
// Only the real SDK owner may assert register_confirmed. Baresip refreshes
// emit REGISTER_OK without REGISTERING; generic mailbox consumers stay strict.
inline SipRuntimeObservation project_native_sdk_batch(SipEventProjection& projection,
 const BaresipEventMailbox::Batch& batch,bool register_confirmed){
 auto empty=batch;empty.size=0;
 auto state=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,empty);
 if(batch.overflow || batch.size>batch.events.size())
  return projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,batch);
 for(std::size_t i=0;i<batch.size;++i){
  const auto& e=batch.events[i];
  const bool live=state.sip==SipStatus::Registered||state.sip==SipStatus::Calling||state.sip==SipStatus::Ringing||state.sip==SipStatus::InCall;
  if(register_confirmed && live && sip_proof_is_current(state,batch.session) &&
     e.kind==BaresipEventKind::RegisterOk && e.session==batch.session && !e.call_id)continue;
  auto one=empty;one.events[0]=e;one.size=1;
  state=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,one);
 }
 const bool live=state.sip==SipStatus::Registered||state.sip==SipStatus::Calling||state.sip==SipStatus::Ringing||state.sip==SipStatus::InCall;
 if(live && !register_confirmed){
  BaresipEventMailbox::Batch revoked{};
  return projection.observe(RuntimeProcess::Failed,SipEvidence::None,revoked);
 }
 return state;
}
}
