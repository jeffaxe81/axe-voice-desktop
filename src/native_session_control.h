#pragma once
#include "sip_event_projection.h"
#include <string>
namespace axv {
enum class NativeCallAction {None,Dial,Answer,Hangup};
struct NativeCallCommand {
 NativeCallAction action=NativeCallAction::None;
 std::uint64_t session=0,call_id=0;
 std::string destination;
};
inline bool native_command_is_current(const NativeCallCommand& c,const SipRuntimeObservation& s){
 if(!sip_proof_is_current(s,c.session) || c.call_id!=s.call_id)return false;
 switch(c.action){
 case NativeCallAction::Dial:return s.sip==SipStatus::Registered && !s.call_id;
 case NativeCallAction::Answer:return s.sip==SipStatus::Ringing && s.call_id;
 case NativeCallAction::Hangup:return s.call_id && (s.sip==SipStatus::Calling||s.sip==SipStatus::Ringing||s.sip==SipStatus::InCall);
 default:return false;
 }
}
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
 return state;
}
}
