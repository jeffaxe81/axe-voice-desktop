#pragma once
#include "sip_runtime_observation.h"
namespace axv {
// Only verified native callbacks may turn the status lamp green.
enum class SipLamp { Idle, Connecting, Connected, Error };
inline SipLamp sip_lamp_state(const SipRuntimeObservation& obs) {
    if (obs.process == RuntimeProcess::Stopped ||
        obs.process == RuntimeProcess::Stopping) return SipLamp::Idle;
    if (obs.process == RuntimeProcess::Failed) return SipLamp::Error;
    if (obs.sip == SipStatus::Failed || obs.sip == SipStatus::Disconnected)
        return SipLamp::Error;
    if (!sip_proof_is_current(obs, obs.session)) return SipLamp::Connecting;
    switch (obs.sip) {
    case SipStatus::Registered:
    case SipStatus::Calling:
    case SipStatus::Ringing:
    case SipStatus::InCall: return SipLamp::Connected;
    case SipStatus::Failed:
    case SipStatus::Disconnected: return SipLamp::Error;
    default: return SipLamp::Connecting;
    }
}
inline const wchar_t* sip_lamp_label(SipLamp status) {
    switch (status) {
    case SipLamp::Idle: return L"Sem conexao";
    case SipLamp::Connecting: return L"Registrando";
    case SipLamp::Connected: return L"Conectado";
    case SipLamp::Error: return L"Falha de conexao";
    }
    return L"Desconhecido";
}
} // namespace axv
