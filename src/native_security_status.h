#pragma once
#include <string>
#include "sip_runtime_observation.h"
namespace axv {
// Policy metadata only. Never account identifiers, secrets or negotiated ciphers.
struct NativeSecurityStatus {
    bool policy_selected=false;
    bool tls_required=false;
    bool running_process=false;
    bool verified_registration=false;
};
inline NativeSecurityStatus native_security_status(bool selected,bool tls,const SipRuntimeObservation& view) {
    const bool registered=view.sip==SipStatus::Registered || view.sip==SipStatus::Calling ||
        view.sip==SipStatus::Ringing || view.sip==SipStatus::InCall;
    return {selected,tls,view.process==RuntimeProcess::Running,
        selected && registered && sip_proof_is_current(view,view.session)};
}
inline std::wstring format_native_security_status(const NativeSecurityStatus& s) {
    std::wstring text=L"SEGURANCA DA SESSAO\nPolitica SIP: ";
    text+=!s.policy_selected?L"sem politica de sessao selecionada":
        s.tls_required?L"TLS obrigatorio; CA confiavel exigida":L"UDP sem criptografia";
    text+=L"\nSessao nativa: ";
    text+=s.running_process?L"em execucao":L"sem sessao em execucao";
    text+=L"\nRegistro SIP atual: ";
    text+=(s.policy_selected && s.running_process && s.verified_registration)?L"verificado":L"sem evidencia atual";
    text+=L"\nMidia: este candidato nao oferece SRTP\nHomologacao real: nao concluida";
    return text;
}
}
