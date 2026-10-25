#pragma once
#include "native_audio_routes.h"
#include "native_session_control.h"
namespace axv {
inline bool native_command_needs_audio_preflight(const NativeCallCommand& command){
 return command.action==NativeCallAction::Dial || command.action==NativeCallAction::Answer ||
        (command.action==NativeCallAction::SetHold && !command.desired);
}
struct NativeAudioCommandDecision {bool allowed=false;NativeAudioRouteStatus audio=NativeAudioRouteStatus::Ready;};
inline NativeAudioCommandDecision admit_native_audio_command(NativeCommandAdmission& admission,
 const NativeCallCommand& command,const SipRuntimeObservation& state,
 const NativeAudioRoute& route,const AudioInventory& inventory){
 if(!native_command_is_current(command,state))return {};
 if(native_command_needs_audio_preflight(command)){
  const auto audio=native_audio_route_status(route,inventory);
  if(audio!=NativeAudioRouteStatus::Ready)return {false,audio};
 }
 return {admission.admit(command,state),NativeAudioRouteStatus::Ready};
}
}
