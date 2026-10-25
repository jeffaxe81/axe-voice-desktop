#include "../src/native_session_control.h"
#include <cstdlib>
#define CHECK(x) do {if(!(x))std::abort();}while(0)
int main(){
 axv::SipEventProjection projection;
 axv::BaresipEventMailbox::Batch b{};b.session=1;b.size=2;
 b.events[0]={axv::BaresipEventKind::Registering,1,0};b.events[1]={axv::BaresipEventKind::RegisterOk,1,0};
 auto s=axv::project_native_sdk_batch(projection,b,true);CHECK(s.sip==axv::SipStatus::Registered);
 b.size=1;b.events[0]={axv::BaresipEventKind::RegisterOk,1,0};
 s=axv::project_native_sdk_batch(projection,b,true);CHECK(s.sip==axv::SipStatus::Registered && s.session==1);
 b.events[0]={axv::BaresipEventKind::CallIncoming,1,1};s=axv::project_native_sdk_batch(projection,b,true);
 auto answer=axv::NativeCallCommand{axv::NativeCallAction::Answer,1,1,{}};
 CHECK(axv::native_command_is_current(answer,s));
 auto next=s;next.call_id=2;CHECK(!axv::native_command_is_current(answer,next));
 next=s;next.session=2;CHECK(!axv::native_command_is_current(answer,next));
 auto hangup=axv::NativeCallCommand{axv::NativeCallAction::Hangup,1,1,{}};
 CHECK(axv::native_command_is_current(hangup,s));next=s;next.call_id=2;CHECK(!axv::native_command_is_current(hangup,next));
 b.events[0]={axv::BaresipEventKind::CallEstablished,1,1};s=axv::project_native_sdk_batch(projection,b,true);CHECK(s.sip==axv::SipStatus::InCall);
 b.events[0]={axv::BaresipEventKind::RegisterOk,1,0};s=axv::project_native_sdk_batch(projection,b,true);CHECK(s.sip==axv::SipStatus::InCall && s.call_id==1);
 s=axv::project_native_sdk_batch(projection,b,false);CHECK(s.evidence==axv::SipEvidence::None);
}
