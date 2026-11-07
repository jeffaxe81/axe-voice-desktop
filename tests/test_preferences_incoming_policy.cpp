// Regression: a modal preferences window may offer incoming SIP call actions
// only when the currently observed and displayed native call identities match.
#include "../src/preferences_incoming_policy.h"
#include <cstdio>
#include <cstdlib>

static void check(bool condition, const char* scenario) {
    if(!condition){ std::fprintf(stderr,"FAIL: %s\n",scenario); std::abort(); }
}
int main() {
    using namespace axv;
    const SipRuntimeObservation incoming{
        RuntimeProcess::Running,SipStatus::Ringing,SipEvidence::VerifiedNativeApi,42,901
    };
    check(preferences_incoming_controls_available(incoming,incoming),
          "verified incoming call should show answers in preferences");
    const SipRuntimeObservation old_call{
        RuntimeProcess::Running,SipStatus::Ringing,SipEvidence::VerifiedNativeApi,42,900
    };
    check(!preferences_incoming_controls_available(incoming,old_call),
          "old call token must not be actionable");
    const SipRuntimeObservation old_session{
        RuntimeProcess::Running,SipStatus::Ringing,SipEvidence::VerifiedNativeApi,41,901
    };
    check(!preferences_incoming_controls_available(incoming,old_session),
          "previous session must not be actionable");
    const SipRuntimeObservation no_proof{
        RuntimeProcess::Running,SipStatus::Ringing,SipEvidence::None,42,901
    };
    check(!preferences_incoming_controls_available(no_proof,incoming),
          "unverified native event must not enable buttons");
    check(!preferences_incoming_controls_available(incoming,no_proof),
          "unverified displayed event must not enable buttons");
    const SipRuntimeObservation no_token{
        RuntimeProcess::Running,SipStatus::Ringing,SipEvidence::VerifiedNativeApi,42,0
    };
    check(!preferences_incoming_controls_available(no_token,no_token),
          "zero call token must not enable actions");
    const SipRuntimeObservation established{
        RuntimeProcess::Running,SipStatus::InCall,SipEvidence::VerifiedNativeApi,42,901
    };
    check(!preferences_incoming_controls_available(established,incoming),
          "already answered call must not show answer/reject");
    const SipRuntimeObservation idle{
        RuntimeProcess::Running,SipStatus::Registered,SipEvidence::VerifiedNativeApi,42,0
    };
    check(!preferences_incoming_controls_available(idle,incoming),
          "closed call must not show actions");
    const SipRuntimeObservation stopped{
        RuntimeProcess::Stopped,SipStatus::Ringing,SipEvidence::VerifiedNativeApi,42,901
    };
    check(!preferences_incoming_controls_available(stopped,incoming),
          "process not running must not show actions");
    for(std::uint64_t i=1;i<=2000;++i){
        const SipRuntimeObservation current{
            RuntimeProcess::Running,SipStatus::Ringing,SipEvidence::VerifiedNativeApi,i,i
        };
        check(preferences_incoming_controls_available(current,current),"current call");
        const SipRuntimeObservation stale{
            RuntimeProcess::Running,SipStatus::Ringing,SipEvidence::VerifiedNativeApi,i+1,i
        };
        check(!preferences_incoming_controls_available(current,stale),"stale session");
    }
    std::puts("Preferences incoming action policy: 2000 allowed + 2000 stale");
}
