#include "../src/native_transfer.h"
#include "../src/native_audio_command_guard.h"
#include <cstdlib>
#define CHECK(x) do{if(!(x))std::abort();}while(0)
int main(){
 axv::SipAccount account{L"pbx.example",L"1001",5060,false};
 CHECK(axv::native_transfer_destination(L"1002",account)=="sip:1002@pbx.example:5060;transport=udp");
 CHECK(axv::valid_native_transfer_uri("sip:1002@pbx.example:5060;transport=udp"));
 for(const auto& bad:{"1002","sip:1002@pbx\r\nX:bad","sip:1002@pbx?X=bad","sip:1002%0d@pbx","sips:1002@pbx;transport=udp","sip:1002@pbx;other=x","sip:1002@pbx;transport=tcp"})CHECK(!axv::valid_native_transfer_uri(bad));
 CHECK(!axv::valid_native_transfer_uri(std::string(513,'a')));
 account.use_tls=true;account.port=5061;
 CHECK(axv::native_transfer_destination(L"sip:1002@pbx.example",account).empty());
 CHECK(axv::native_transfer_destination(L"1002",account)=="sips:1002@pbx.example:5061;transport=tls");
 axv::SipRuntimeObservation call{axv::RuntimeProcess::Running,axv::SipStatus::InCall,axv::SipEvidence::VerifiedNativeApi,2,3,3};
 axv::NativeCallCommand command{axv::NativeCallAction::Transfer,2,3,"sip:1002@pbx.example"};
 axv::NativeCommandAdmission admission;
 CHECK(axv::admit_native_audio_command(admission,command,call,{},{}).allowed);
 CHECK(!admission.admit(command,call));
 auto next=call;next.call_id=4;CHECK(!axv::native_command_is_current(command,next));
 next=call;next.session=3;CHECK(!axv::native_command_is_current(command,next));
 next=call;next.sip=axv::SipStatus::Ringing;CHECK(!axv::native_command_is_current(command,next));
 command.call_id=4;next=call;next.call_id=4;CHECK(admission.admit(command,next));
}
