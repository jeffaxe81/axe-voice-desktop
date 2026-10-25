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
 CHECK(axv::native_media_controls_current(media,state));
 state.sip=axv::SipStatus::Ringing;CHECK(!axv::native_command_is_current(mute,state));CHECK(!axv::native_media_controls_current(media,state));
 state.sip=axv::SipStatus::InCall;state.call_id=4;CHECK(!axv::native_media_controls_current(media,state));
 state.call_id=3;state.evidence=axv::SipEvidence::None;CHECK(!axv::native_command_is_current(hold,state));CHECK(!axv::native_media_controls_current(media,state));
}
