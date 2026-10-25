#pragma once
#include "sip_runtime_observation.h"
#include "native_audio_routes.h"
namespace axv {
// SDK local flags only: local_hold does not confirm remote SIP acceptance.
enum class NativeTransferState {None,Submitted,Failed};
struct NativeCallMediaState {
 std::uint64_t session=0,call_id=0;
 bool available=false,muted=false,local_hold=false;
 int hold_request_error=0;
 NativeTransferState transfer=NativeTransferState::None;
};
inline void reset_native_call_feedback(const NativeCallMediaState& previous,const NativeCallMediaState& current,
 int& hold_error,bool& mute_error,NativeAudioRouteStatus& audio_error){
 if(previous.session!=current.session || previous.call_id!=current.call_id){
  hold_error=0;mute_error=false;audio_error=NativeAudioRouteStatus::Ready;
 }
}
inline bool native_media_controls_current(const NativeCallMediaState& media,const SipRuntimeObservation& call){
 return media.available && media.call_id && media.call_id==call.call_id &&
        call.sip==SipStatus::InCall && sip_proof_is_current(call,media.session);
}
}
