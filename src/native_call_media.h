#pragma once
#include "sip_runtime_observation.h"
namespace axv {
// SDK local flags only: local_hold does not confirm remote SIP acceptance.
struct NativeCallMediaState {
 std::uint64_t session=0,call_id=0;
 bool available=false,muted=false,local_hold=false;
 int hold_request_error=0;
};
inline bool native_media_controls_current(const NativeCallMediaState& media,const SipRuntimeObservation& call){
 return media.available && media.call_id && media.call_id==call.call_id &&
        call.sip==SipStatus::InCall && sip_proof_is_current(call,media.session);
}
}
