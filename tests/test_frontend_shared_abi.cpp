#include "../src/frontend_shared/axv_frontend_api.h"
#include "../src/sip_runtime_observation.h"
// MSVC Release defines NDEBUG; keep ABI contract checks live in CI.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>

using axv::SipStatus;
using axv::RuntimeProcess;
using axv::SipEvidence;

AxvFrontendObservation observation(SipStatus status, SipEvidence proof,
                                    uint64_t session, uint64_t call,
                                    uint64_t current, bool dial=true) {
    return {sizeof(AxvFrontendObservation),
            static_cast<uint32_t>(RuntimeProcess::Running),
            static_cast<uint32_t>(status),static_cast<uint32_t>(proof),
            session,call,current,dial?1u:0u};
}
AxvFrontendSnapshot project(const AxvFrontendObservation& input) {
    AxvFrontendSnapshot out{};
    const int rc=axv_frontend_project_snapshot(&input,sizeof(input),&out,sizeof(out));
    assert(rc==AXV_FRONTEND_OK);
    assert(out.abi_version==1 && out.struct_size==sizeof(out));
    assert(out.backend_attached==0);
    return out;
}
int main(){
    assert(axv_frontend_abi_version()==AXV_FRONTEND_ABI_VERSION);
    AxvFrontendSnapshot live{};
    assert(axv_frontend_get_snapshot(&live,sizeof(live))==AXV_FRONTEND_OK);
    assert(live.backend_attached==0 && live.connection==0 &&
           live.can_dial==0 && live.can_answer==0 &&
           live.can_hangup==0 && live.can_secondary==0);
    assert(axv_frontend_get_snapshot(nullptr,sizeof(live))==AXV_FRONTEND_ERROR_INVALID);
    assert(axv_frontend_get_snapshot(&live,sizeof(live)-1)==AXV_FRONTEND_ERROR_INVALID);
    assert(axv_frontend_project_snapshot(nullptr,0,&live,sizeof(live))==AXV_FRONTEND_ERROR_INVALID);
    AxvFrontendObservation invalid{};
    assert(axv_frontend_project_snapshot(&invalid,sizeof(invalid),&live,sizeof(live))==AXV_FRONTEND_ERROR_INVALID);
    auto registered=observation(SipStatus::Registered,SipEvidence::VerifiedNativeApi,4,0,4);
    auto state=project(registered);
    assert(state.connection==2 && state.can_dial==1 && state.view==0);
    auto ringing=observation(SipStatus::Ringing,SipEvidence::VerifiedNativeApi,4,12,4);
    state=project(ringing);
    assert(state.connection==2 && state.view==1 && state.can_answer && state.can_hangup);
    auto calling=observation(SipStatus::Calling,SipEvidence::VerifiedNativeApi,4,12,4);
    state=project(calling);
    assert(state.view==2 && state.can_hangup && !state.can_answer);
    auto connected=observation(SipStatus::InCall,SipEvidence::VerifiedNativeApi,4,12,4);
    state=project(connected);
    assert(state.view==3 && state.can_hangup && state.can_secondary);
    auto stale=observation(SipStatus::Ringing,SipEvidence::VerifiedNativeApi,4,12,5);
    state=project(stale);
    assert(state.connection==0 && !state.can_answer && state.call_id==0);
    auto unproven=observation(SipStatus::Ringing,SipEvidence::None,4,12,4);
    state=project(unproven);
    assert(state.connection==0 && !state.can_answer);
    auto missing_token=observation(SipStatus::Ringing,SipEvidence::VerifiedNativeApi,4,0,4);
    state=project(missing_token);
    assert(!state.can_answer && !state.can_hangup);
    for(uint64_t i=1;i<=2000;++i){
        auto observed=observation(SipStatus::Ringing,SipEvidence::VerifiedNativeApi,i,i,i);
        assert(project(observed).can_answer==1);
        observed.current_session=i+1;
        assert(project(observed).can_answer==0);
    }
    registered.process=999;
    assert(axv_frontend_project_snapshot(&registered,sizeof(registered),&live,sizeof(live))==
           AXV_FRONTEND_ERROR_INVALID);
    std::puts("AXV frontend bridge ABI: 2,000 verified and 2,000 stale states passed");
    return 0;
}
