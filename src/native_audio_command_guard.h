#pragma once
#include "native_audio_routes.h"
#include "native_session_control.h"
namespace axv {
struct NativeAudioCommandDecision {bool allowed=false;NativeAudioRouteStatus audio=NativeAudioRouteStatus::Ready;};
inline NativeAudioCommandDecision admit_native_audio_command(NativeCommandAdmission& admission,
 const NativeCallCommand& command,const SipRuntimeObservation& state,
 const NativeAudioRoute& route,const AudioInventory& inventory){
 if(!native_command_is_current(command,state))return {};
 if(command.action==NativeCallAction::Dial || command.action==NativeCallAction::Answer){
  const auto audio=native_audio_route_status(route,inventory);
  if(audio!=NativeAudioRouteStatus::Ready)return {false,audio};
 }
 return {admission.admit(command,state),NativeAudioRouteStatus::Ready};
}
}
