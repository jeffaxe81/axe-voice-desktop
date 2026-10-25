#include "../src/incoming_ringtone_policy.h"
#include "../src/native_session_control.h"
#include <cstdlib>
#define CHECK(x) do{if(!(x))std::abort();}while(0)
int main(){
 axv::SipRuntimeObservation call{axv::RuntimeProcess::Running,axv::SipStatus::Ringing,axv::SipEvidence::VerifiedNativeApi,1,2,2};
 axv::IncomingRingtonePolicy policy;
 CHECK(policy.request(call,L"selected-output").active());
 CHECK(!policy.request(call,L"selected-output",true).active());
 CHECK(policy.request(call,L"selected-output",false).active());
 CHECK(axv::native_displayed_call_command_current(axv::NativeCallAction::Answer,call,call));
 auto replacement=call;replacement.call_id=3;
 CHECK(!axv::native_displayed_call_command_current(axv::NativeCallAction::Answer,call,replacement));
 CHECK(!axv::native_displayed_call_command_current(axv::NativeCallAction::Hangup,call,replacement));
 CHECK(axv::native_displayed_call_command_current(axv::NativeCallAction::Hangup,call,call));
 CHECK(policy.request(call,L"").endpoint_id.empty());
 auto invalid=call;invalid.evidence=axv::SipEvidence::None;CHECK(!policy.request(invalid,L"id").active());
 invalid=call;invalid.sip=axv::SipStatus::Calling;CHECK(!policy.request(invalid,L"id").active());
 invalid=call;invalid.process=axv::RuntimeProcess::Stopping;CHECK(!policy.request(invalid,L"id").active());
 invalid=call;invalid.call_id=0;CHECK(!policy.request(invalid,L"id").active());
 CHECK(!policy.request(call,std::wstring(4097,L'x')).active());
 policy.silence(call);CHECK(!policy.request(call,L"selected-output").active());
 call.call_id=3;CHECK(policy.request(call,L"selected-output").active());
 call.session=2;call.call_id=2;CHECK(policy.request(call,L"selected-output").active());
 std::vector<axv::AudioEndpoint> outputs{{L"communication",L"Voice",true,false},{L"multimedia",L"Ring",false,true}};
 CHECK(axv::fixed_native_ringtone_selection(outputs,L"").index==1);
 CHECK(axv::fixed_native_ringtone_selection(outputs,L"").selected_id.empty());
 CHECK(!axv::fixed_native_ringtone_selection(outputs,L"missing").available());
 CHECK(axv::fixed_native_ringtone_selection(outputs,L"missing").selected_id==L"missing");
}
