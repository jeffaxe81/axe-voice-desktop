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

 // Exercise all possible 8-bit keypad inputs. Only ASCII digits, * and #
 // may be emitted over the selected SIP INFO DTMF policy.
 for(unsigned k=0;k<256;++k) {
  const wchar_t key=static_cast<wchar_t>(k);
  const bool expected=(k>=static_cast<unsigned>('0') && k<=static_cast<unsigned>('9'))||k=='*'||k=='#';
  CHECK(axv::valid_native_dtmf_key(key)==expected);
 }
 // DTMF must be revoked for stale media, missing evidence, rejected call
 // transfer or local hold. This is synthetic command admission, not audio.
 for(std::uint64_t i=1;i<=1000;++i) {
  const axv::SipRuntimeObservation c{axv::RuntimeProcess::Running,
    axv::SipStatus::InCall,axv::SipEvidence::VerifiedNativeApi,i+10,i+20,i+19};
  const axv::NativeCallMediaState m{i+10,i+20,true};
  axv::NativeDtmfMode keypad;
  CHECK(keypad.toggle(m,c)&&keypad.active(m,c));
  auto stale=c;stale.session++;
  CHECK(!keypad.active(m,stale));
  keypad.synchronize(m,stale);CHECK(!keypad.armed());
  CHECK(keypad.toggle(m,c));
  auto held=m;held.local_hold=true;CHECK(!keypad.active(held,c));
  keypad.synchronize(held,c);CHECK(!keypad.armed());
  auto transfer=m;transfer.transfer=axv::NativeTransferState::Submitted;
  CHECK(!keypad.toggle(transfer,c));
  auto unverified=c;unverified.evidence=axv::SipEvidence::None;
  CHECK(!keypad.toggle(m,unverified));
  auto no_audio=m;no_audio.available=false;
  CHECK(!keypad.toggle(no_audio,c));
  auto altered=c;altered.call_id++;
  CHECK(!keypad.toggle(m,altered));
  CHECK(keypad.toggle(m,c)&&keypad.toggle(m,c)&&!keypad.armed());
 }
}
