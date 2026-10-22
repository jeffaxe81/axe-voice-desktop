#pragma once
#include "sip_engine.h"
#include "call_policy.h"
#include <cstdint>

namespace axv {
// VerifiedNativeApi must be issued only by a future audited in-process adapter.
// Process lifetime and textual logs cannot provide SIP registration evidence.
enum class SipEvidence { None, VerifiedNativeApi };
enum class RuntimeProcess { Stopped, Running, Stopping, Failed };
struct SipRuntimeObservation {
    RuntimeProcess process = RuntimeProcess::Stopped;
    SipStatus sip = SipStatus::Unavailable;
    SipEvidence evidence = SipEvidence::None;
    std::uint64_t session = 0;
    // Opaque monotonic call token assigned by a future native adapter.
    // Never derive this from a phone number or expose a raw pointer.
    std::uint64_t call_id = 0;
    std::uint64_t last_call_id = 0;
};
inline bool sip_proof_is_current(const SipRuntimeObservation& observation,
                                 std::uint64_t current_session) {
    return observation.process == RuntimeProcess::Running &&
           observation.evidence == SipEvidence::VerifiedNativeApi &&
           current_session != 0 && observation.session == current_session;
}
inline SipStatus effective_sip_status(const SipRuntimeObservation& observation,
                                      std::uint64_t current_session) {
    return sip_proof_is_current(observation, current_session)
        ? observation.sip : SipStatus::Unavailable;
}
inline bool may_use_call_control(const SipRuntimeObservation& observation,
                                 std::uint64_t current_session, CallAction action) {
    return can_apply_call_action(effective_sip_status(observation, current_session), action);
}
inline const wchar_t* runtime_status_banner(const SipRuntimeObservation& observation,
                                             std::uint64_t current_session) {
    switch (observation.process) {
    case RuntimeProcess::Stopped: return L"MOTOR PARADO  |  SIP NAO REGISTRADO";
    case RuntimeProcess::Stopping: return L"MOTOR ENCERRANDO  |  SIP NAO REGISTRADO";
    case RuntimeProcess::Failed: return L"FALHA NO MOTOR  |  SIP NAO REGISTRADO";
    case RuntimeProcess::Running: break;
    }
    if (!sip_proof_is_current(observation, current_session))
        return L"MOTOR ATIVO  |  REGISTRO SIP NAO CONFIRMADO";
    switch (observation.sip) {
    case SipStatus::Registered: return L"SIP REGISTRADO  |  EVENTO NATIVO VALIDADO";
    case SipStatus::Calling: return L"CHAMADA ORIGINANDO  |  EVENTO NATIVO VALIDADO";
    case SipStatus::Ringing: return L"CHAMADA ENTRANTE  |  EVENTO NATIVO VALIDADO";
    case SipStatus::InCall: return L"EM CHAMADA  |  EVENTO NATIVO VALIDADO";
    case SipStatus::Registering: return L"REGISTRO SIP EM ANDAMENTO";
    case SipStatus::Failed: return L"FALHA NO REGISTRO SIP";
    case SipStatus::Disconnected: return L"SIP DESCONECTADO";
    case SipStatus::Unavailable: return L"SIP INDISPONIVEL";
    }
    return L"REGISTRO SIP NAO CONFIRMADO";
}
inline bool is_stale_session(std::uint64_t event_session, std::uint64_t current_session) {
    return event_session == 0 || event_session != current_session;
}
} // namespace axv
