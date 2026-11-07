// Real state projector contract, exercised with synthetic SIP runtime snapshots.
#include "../src/essential_call_actions.h"
#include <cstdio>
#include <cstdlib>

namespace {
using axv::EssentialCallView;
using axv::EssentialCallActions;
using axv::RuntimeProcess;
using axv::SipStatus;
using axv::SipEvidence;
using axv::SipRuntimeObservation;

void expect(const char* case_name, const SipRuntimeObservation& observation,
            std::uint64_t current_session, bool dial_allowed,
            EssentialCallView view, bool dial, bool answer,
            bool hangup, bool secondary) {
    const auto result=axv::essential_call_actions(observation,current_session,dial_allowed);
    if (result.view!=view || result.dial!=dial || result.answer!=answer ||
        result.hangup!=hangup || result.secondary!=secondary) {
        std::fprintf(stderr,"FAIL %s: view=%d dial=%d answer=%d hangup=%d secondary=%d\n",
            case_name,static_cast<int>(result.view),int(result.dial),
            int(result.answer),int(result.hangup),int(result.secondary));
        std::abort();
    }
}
}
int main() {
    using V=axv::EssentialCallView;
    const SipRuntimeObservation stopped{RuntimeProcess::Stopped,SipStatus::Disconnected,
                                        SipEvidence::None,0,0};
    expect("stopped",stopped,0,true,V::Idle,false,false,false,false);
    const SipRuntimeObservation registering{RuntimeProcess::Running,SipStatus::Registering,
                                            SipEvidence::VerifiedNativeApi,15,0};
    expect("registering",registering,15,true,V::Idle,false,false,false,false);
    const SipRuntimeObservation registered{RuntimeProcess::Running,SipStatus::Registered,
                                           SipEvidence::VerifiedNativeApi,15,0};
    expect("registered dial",registered,15,true,V::Idle,true,false,false,false);
    expect("registered no destination",registered,15,false,V::Idle,false,false,false,false);
    const SipRuntimeObservation invalid_registered{RuntimeProcess::Running,SipStatus::Registered,
                                                   SipEvidence::VerifiedNativeApi,15,100};
    expect("registered with occupied call token",invalid_registered,15,true,
           V::Idle,false,false,false,false);
    const SipRuntimeObservation incoming{RuntimeProcess::Running,SipStatus::Ringing,
                                         SipEvidence::VerifiedNativeApi,15,100};
    expect("incoming",incoming,15,true,V::Incoming,false,true,true,false);
    const SipRuntimeObservation outgoing{RuntimeProcess::Running,SipStatus::Calling,
                                         SipEvidence::VerifiedNativeApi,15,100};
    expect("outgoing",outgoing,15,true,V::Outgoing,false,false,true,false);
    const SipRuntimeObservation established{RuntimeProcess::Running,SipStatus::InCall,
                                            SipEvidence::VerifiedNativeApi,15,100};
    expect("established",established,15,true,V::Connected,false,false,true,true);
    const SipRuntimeObservation unverified{RuntimeProcess::Running,SipStatus::Ringing,
                                           SipEvidence::None,15,100};
    expect("unverified ringing",unverified,15,true,V::Idle,false,false,false,false);
    expect("stale ring",incoming,16,true,V::Idle,false,false,false,false);
    expect("stale connected",established,16,true,V::Idle,false,false,false,false);
    const SipRuntimeObservation zero_call{RuntimeProcess::Running,SipStatus::Ringing,
                                          SipEvidence::VerifiedNativeApi,15,0};
    expect("incoming zero token",zero_call,15,true,V::Idle,false,false,false,false);
    const SipRuntimeObservation disconnect{RuntimeProcess::Running,SipStatus::Disconnected,
                                           SipEvidence::VerifiedNativeApi,15,0};
    expect("transport loss",disconnect,15,true,V::Idle,false,false,false,false);
    const SipRuntimeObservation failed{RuntimeProcess::Failed,SipStatus::InCall,
                                       SipEvidence::VerifiedNativeApi,15,100};
    expect("failed process",failed,15,true,V::Idle,false,false,false,false);
    for(int i=0;i<2000;++i) {
        const bool incoming_call=i<1000;
        const auto s=SipRuntimeObservation{RuntimeProcess::Running,
                incoming_call?SipStatus::Ringing:SipStatus::Calling,
                SipEvidence::VerifiedNativeApi,static_cast<std::uint64_t>(i+1),
                static_cast<std::uint64_t>(i+1)};
        expect("batch good",s,s.session,true,incoming_call?V::Incoming:V::Outgoing,
               false,incoming_call,true,false);
        expect("batch stale",s,s.session+1,true,V::Idle,false,false,false,false);
    }
    std::puts("Essential SIP action projector: 2,000 trusted + 2,000 stale simulated states OK");
}
