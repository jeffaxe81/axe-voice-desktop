#define AXV_FRONTEND_BRIDGE_BUILD
#include "axv_frontend_api.h"
#include "../essential_call_actions.h"
#include <cstring>

namespace {
int32_t fail_closed(AxvFrontendSnapshot* out, uint32_t out_size) noexcept {
    if (out && out_size == sizeof(AxvFrontendSnapshot)) {
        std::memset(out,0,sizeof(*out));
        out->struct_size=sizeof(*out);
        out->abi_version=AXV_FRONTEND_ABI_VERSION;
    }
    return AXV_FRONTEND_ERROR_INVALID;
}
void make_offline(AxvFrontendSnapshot* out) noexcept {
    std::memset(out,0,sizeof(*out));
    out->struct_size=sizeof(*out);
    out->abi_version=AXV_FRONTEND_ABI_VERSION;
    out->backend_attached=0;
}
bool valid_input(const AxvFrontendObservation& input) noexcept {
    return input.struct_size==sizeof(AxvFrontendObservation) &&
           input.process<=static_cast<uint32_t>(axv::RuntimeProcess::Failed) &&
           input.sip<=static_cast<uint32_t>(axv::SipStatus::Failed) &&
           input.evidence<=static_cast<uint32_t>(axv::SipEvidence::VerifiedNativeApi) &&
           input.dial_authorized<=1;
}
}

extern "C" uint32_t axv_frontend_abi_version(void) {
    return AXV_FRONTEND_ABI_VERSION;
}
extern "C" int32_t axv_frontend_get_snapshot(AxvFrontendSnapshot* out,uint32_t size) {
    if(!out || size!=sizeof(*out))return AXV_FRONTEND_ERROR_INVALID;
    make_offline(out);
    // Nothing has attached a Baresip owner to this bridge yet.
    return AXV_FRONTEND_OK;
}
extern "C" int32_t axv_frontend_project_snapshot(
    const AxvFrontendObservation* input,uint32_t input_size,
    AxvFrontendSnapshot* out,uint32_t out_size) {
    if(!out || out_size!=sizeof(*out))return AXV_FRONTEND_ERROR_INVALID;
    make_offline(out);
    if(!input || input_size!=sizeof(*input) || !valid_input(*input))
        return fail_closed(out,out_size);

    const axv::SipRuntimeObservation observation{
        static_cast<axv::RuntimeProcess>(input->process),
        static_cast<axv::SipStatus>(input->sip),
        static_cast<axv::SipEvidence>(input->evidence),
        input->session,input->call_id
    };
    const auto signal=axv::essential_connection_signal(observation,input->current_session);
    const auto actions=axv::essential_call_actions(
        observation,input->current_session,input->dial_authorized==1);
    out->connection=static_cast<uint32_t>(signal);
    out->view=static_cast<uint32_t>(actions.view);
    out->can_dial=actions.dial;
    out->can_answer=actions.answer;
    out->can_hangup=actions.hangup;
    out->can_secondary=actions.secondary;
    // Opaque session IDs are only published when a verified SIP proof exists.
    if(axv::sip_proof_is_current(observation,input->current_session)) {
        out->session=observation.session;
        out->call_id=observation.call_id;
    }
    // Not a live engine. backend_attached remains 0 by design.
    return AXV_FRONTEND_OK;
}
