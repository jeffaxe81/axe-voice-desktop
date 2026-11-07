#define AXV_FRONTEND_BRIDGE_BUILD
#include "axv_frontend_api.h"
#include "../essential_call_actions.h"
#include "../destination_feedback.h"
#ifdef AXV_FRONTEND_NATIVE_SIP
#include "axv_native_host.h"
#endif
#include <string>
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

extern "C" int32_t axv_frontend_validate_destination(
    const wchar_t* destination,uint32_t length) {
    if(!destination)return AXV_FRONTEND_ERROR_INVALID;
    if(length>128)return AXV_DEST_INVALID;
    if(length==0)return AXV_DEST_EMPTY;
    for(uint32_t i=0;i<length;++i)
        if(destination[i]==L'\0')return AXV_DEST_INVALID;
    const auto feedback=axv::evaluate_destination(
        std::wstring(destination,destination+length));
    if(feedback.kind==axv::DestinationFeedbackKind::Valid)
        return AXV_DEST_VALID;
    if(feedback.kind==axv::DestinationFeedbackKind::Empty)
        return AXV_DEST_EMPTY;
    return AXV_DEST_INVALID;
}

extern "C" int32_t axv_frontend_connect_saved(void) {
#ifdef AXV_FRONTEND_NATIVE_SIP
    return axv::native_host_connect_saved();
#else
    return AXV_FRONTEND_ERROR_UNAVAILABLE;
#endif
}
extern "C" int32_t axv_frontend_disconnect(void) {
#ifdef AXV_FRONTEND_NATIVE_SIP
    return axv::native_host_disconnect();
#else
    return AXV_FRONTEND_ERROR_UNAVAILABLE;
#endif
}
extern "C" int32_t axv_frontend_dial(
    const wchar_t* destination,uint32_t length,uint64_t expected_session) {
    if(!destination || length==0 || length>128 ||
       axv_frontend_validate_destination(destination,length)!=AXV_DEST_VALID)
        return AXV_FRONTEND_ERROR_INVALID;
#ifdef AXV_FRONTEND_NATIVE_SIP
    return axv::native_host_dial(
        std::wstring(destination,destination+length),expected_session);
#else
    (void)expected_session;
    return AXV_FRONTEND_ERROR_UNAVAILABLE;
#endif
}
extern "C" int32_t axv_frontend_answer(
    uint64_t expected_session,uint64_t expected_call_id) {
#ifdef AXV_FRONTEND_NATIVE_SIP
    return axv::native_host_answer(expected_session,expected_call_id);
#else
    (void)expected_session;(void)expected_call_id;
    return AXV_FRONTEND_ERROR_UNAVAILABLE;
#endif
}
extern "C" int32_t axv_frontend_hangup(
    uint64_t expected_session,uint64_t expected_call_id) {
#ifdef AXV_FRONTEND_NATIVE_SIP
    return axv::native_host_hangup(expected_session,expected_call_id);
#else
    (void)expected_session;(void)expected_call_id;
    return AXV_FRONTEND_ERROR_UNAVAILABLE;
#endif
}

extern "C" uint32_t axv_frontend_abi_version(void) {
    return AXV_FRONTEND_ABI_VERSION;
}
extern "C" int32_t axv_frontend_get_snapshot(AxvFrontendSnapshot* out,uint32_t size) {
    if(!out || size!=sizeof(*out))return AXV_FRONTEND_ERROR_INVALID;
    make_offline(out);
#ifdef AXV_FRONTEND_NATIVE_SIP
    return axv::native_host_get_snapshot(out);
#else
    // The normal WinUI3 Preview cannot make a real call.
    return AXV_FRONTEND_OK;
#endif
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
