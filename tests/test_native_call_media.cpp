#include "../src/native_call_media.h"
#include "../src/native_session_control.h"
#include "../src/native_audio_command_guard.h"
#include <cstdlib>
#define CHECK(x) do{if(!(x))std::abort();}while(0)
int main(){
 axv::SipRuntimeObservation state{axv::RuntimeProcess::Running,axv::SipStatus::InCall,axv::SipEvidence::VerifiedNativeApi,2,3,3};
 axv::NativeCallCommand mute{axv::NativeCallAction::SetMute,2,3,{},true};
 axv::NativeCallCommand hold{axv::NativeCallAction::SetHold,2,3,{},true};
 axv::NativeCommandAdmission admission;
 CHECK(!axv::native_command_needs_audio_preflight(mute));
 CHECK(!axv::native_command_needs_audio_preflight(hold));
 CHECK(axv::admit_native_audio_command(admission,mute,state,{},{}).allowed);
 CHECK(axv::admit_native_audio_command(admission,hold,state,{},{}).allowed);
 auto resume=hold;resume.desired=false;
 CHECK(axv::native_command_needs_audio_preflight(resume));
 CHECK(!axv::admit_native_audio_command(admission,resume,state,{},{}).allowed);
 CHECK(axv::native_command_is_current(mute,state));CHECK(axv::native_command_is_current(hold,state));
 auto old=mute;old.call_id=2;CHECK(!axv::native_command_is_current(old,state));
 old=mute;old.session=1;CHECK(!axv::native_command_is_current(old,state));
 axv::NativeCallMediaState media{2,3,true,true,true};
 int hold_error=5;bool mute_error=true;auto audio_error=axv::NativeAudioRouteStatus::MissingPlayback;
 axv::reset_native_call_feedback(media,media,hold_error,mute_error,audio_error);
 CHECK(hold_error==5 && mute_error && audio_error==axv::NativeAudioRouteStatus::MissingPlayback);
 axv::reset_native_call_feedback(media,{},hold_error,mute_error,audio_error);
 CHECK(hold_error==0 && !mute_error && audio_error==axv::NativeAudioRouteStatus::Ready);
 auto replacement=media;replacement.call_id=4;
 hold_error=5;mute_error=true;audio_error=axv::NativeAudioRouteStatus::MissingCapture;
 axv::reset_native_call_feedback(media,replacement,hold_error,mute_error,audio_error);
 CHECK(hold_error==0 && !mute_error && audio_error==axv::NativeAudioRouteStatus::Ready);
 CHECK(axv::native_media_controls_current(media,state));
 state.sip=axv::SipStatus::Ringing;CHECK(!axv::native_command_is_current(mute,state));CHECK(!axv::native_media_controls_current(media,state));
 state.sip=axv::SipStatus::InCall;state.call_id=4;CHECK(!axv::native_media_controls_current(media,state));
 state.call_id=3;state.evidence=axv::SipEvidence::None;CHECK(!axv::native_command_is_current(hold,state));CHECK(!axv::native_media_controls_current(media,state));
}
