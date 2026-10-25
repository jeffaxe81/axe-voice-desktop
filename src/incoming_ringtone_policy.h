#pragma once
#include "sip_runtime_observation.h"
#include "audio_selection_feedback.h"
namespace axv {
struct IncomingRingtoneRequest {
 std::uint64_t session=0,call_id=0;
 std::wstring endpoint_id;
 bool active()const{return session && call_id;}
};
inline bool valid_ringtone_endpoint(const std::wstring& id){return id.size()<=4096 && id.find(L'\0')==std::wstring::npos;}
inline bool verified_incoming_call(const SipRuntimeObservation& call){
 return call.call_id && call.sip==SipStatus::Ringing && sip_proof_is_current(call,call.session);
}
class IncomingRingtonePolicy {
 std::uint64_t silenced_session_=0,silenced_call_=0;
public:
 IncomingRingtoneRequest request(const SipRuntimeObservation& call,const std::wstring& id)const{
  if(!verified_incoming_call(call) || !valid_ringtone_endpoint(id) ||
     (call.session==silenced_session_ && call.call_id==silenced_call_))return {};
  return {call.session,call.call_id,id};
 }
 void silence(const SipRuntimeObservation& call){
  if(verified_incoming_call(call)){silenced_session_=call.session;silenced_call_=call.call_id;}
 }
};
inline AudioSelectionRefresh fixed_native_ringtone_selection(const std::vector<AudioEndpoint>& outputs,const std::wstring& id){
 for(std::size_t i=0;i<outputs.size();++i)
  if(id.empty()?outputs[i].default_multimedia:outputs[i].id==id)return {static_cast<int>(i),id,false};
 return {-1,id,false};
}
}
