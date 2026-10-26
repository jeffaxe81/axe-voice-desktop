#include "../src/native_dtmf.h"
#include "../src/native_session_control.h"
#include <cstdlib>
#define CHECK(x) do{if(!(x))std::abort();}while(0)
int main(){
 for(auto key:std::wstring(L"0123456789*#"))CHECK(axv::valid_native_dtmf_key(key));
 for(auto key:{L'\0',L'\n',L'A',L'+',L' ',L'１'})CHECK(!axv::valid_native_dtmf_key(key));
 axv::SipRuntimeObservation call{axv::RuntimeProcess::Running,axv::SipStatus::InCall,axv::SipEvidence::VerifiedNativeApi,1,2,2};
 axv::NativeCallMediaState media{1,2,true};
 CHECK(axv::native_dtmf_controls_current(media,call));
 axv::NativeDtmfMode mode;CHECK(!mode.armed());
 CHECK(mode.toggle(media,call) && mode.active(media,call));
 CHECK(mode.toggle(media,call) && !mode.armed());
 CHECK(mode.toggle(media,call));
 auto next=call;next.call_id=3;mode.synchronize(media,next);CHECK(!mode.armed());
 CHECK(!mode.toggle(media,next) && !mode.armed());
 CHECK(mode.toggle(media,call));auto held=media;held.local_hold=true;mode.synchronize(held,call);CHECK(!mode.armed());
 auto bad=media;bad.call_id=3;CHECK(!axv::native_dtmf_controls_current(bad,call));
 bad=media;bad.session=2;CHECK(!axv::native_dtmf_controls_current(bad,call));
 bad=media;bad.local_hold=true;CHECK(!axv::native_dtmf_controls_current(bad,call));
 bad=media;bad.transfer=axv::NativeTransferState::Submitted;CHECK(!axv::native_dtmf_controls_current(bad,call));
 auto stale=call;stale.evidence=axv::SipEvidence::None;CHECK(!axv::native_dtmf_controls_current(media,stale));
 stale=call;stale.sip=axv::SipStatus::Ringing;CHECK(!axv::native_dtmf_controls_current(media,stale));
 axv::NativeCallCommand digit{axv::NativeCallAction::Dtmf,1,2,"5"};
 axv::NativeCommandAdmission admission;CHECK(admission.admit(digit,call));CHECK(admission.admit(digit,call));
 digit.destination="55";CHECK(!admission.admit(digit,call));
 digit.destination="A";CHECK(!admission.admit(digit,call));
 digit.destination="5";digit.call_id=3;CHECK(!admission.admit(digit,call));
}
