#pragma once
#include "native_call_media.h"
namespace axv {
inline bool valid_native_dtmf_key(wchar_t key){return (key>=L'0' && key<=L'9') || key==L'*' || key==L'#';}
inline bool native_dtmf_controls_current(const NativeCallMediaState& media,const SipRuntimeObservation& call){
 return native_media_controls_current(media,call) && !media.local_hold && media.transfer==NativeTransferState::None;
}
class NativeDtmfMode {
 std::uint64_t session_=0,call_=0;
public:
 bool armed()const{return session_ && call_;}
 bool active(const NativeCallMediaState& media,const SipRuntimeObservation& call)const{
  return armed() && session_==media.session && call_==media.call_id && native_dtmf_controls_current(media,call);
 }
 void reset(){session_=call_=0;}
 void synchronize(const NativeCallMediaState& media,const SipRuntimeObservation& call){if(!active(media,call))reset();}
 bool toggle(const NativeCallMediaState& media,const SipRuntimeObservation& call){
  if(active(media,call)){reset();return true;}
  reset();if(!native_dtmf_controls_current(media,call))return false;
  session_=media.session;call_=media.call_id;return true;
 }
};

}
