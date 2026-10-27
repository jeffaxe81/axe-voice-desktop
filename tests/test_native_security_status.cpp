#include "../src/native_security_status.h"
#include "../src/sip_runtime_observation.h"
#include <iostream>
#include <string>

// These checks stay active in Release builds. The report must never turn a
// selected TLS policy or stale registration bit into proof of a live session.
int main() {
    struct Case {
        axv::NativeSecurityStatus state;
        const wchar_t* policy;
        const wchar_t* process;
        const wchar_t* registration;
    };
    const Case cases[] = {
        {{false,false,false,false}, L"sem politica de sessao selecionada", L"sem sessao em execucao", L"sem evidencia atual"},
        {{false,false,false,true }, L"sem politica de sessao selecionada", L"sem sessao em execucao", L"sem evidencia atual"},
        {{false,false,true, false}, L"sem politica de sessao selecionada", L"em execucao", L"sem evidencia atual"},
        {{false,false,true, true }, L"sem politica de sessao selecionada", L"em execucao", L"sem evidencia atual"},
        {{false,true, false,false}, L"sem politica de sessao selecionada", L"sem sessao em execucao", L"sem evidencia atual"},
        {{false,true, false,true }, L"sem politica de sessao selecionada", L"sem sessao em execucao", L"sem evidencia atual"},
        {{false,true, true, false}, L"sem politica de sessao selecionada", L"em execucao", L"sem evidencia atual"},
        {{false,true, true, true }, L"sem politica de sessao selecionada", L"em execucao", L"sem evidencia atual"},
        {{true, false,false,false}, L"UDP sem criptografia", L"sem sessao em execucao", L"sem evidencia atual"},
        {{true, false,false,true }, L"UDP sem criptografia", L"sem sessao em execucao", L"sem evidencia atual"},
        {{true, false,true, false}, L"UDP sem criptografia", L"em execucao", L"sem evidencia atual"},
        {{true, false,true, true }, L"UDP sem criptografia", L"em execucao", L"verificado"},
        {{true, true, false,false}, L"TLS obrigatorio; CA confiavel exigida", L"sem sessao em execucao", L"sem evidencia atual"},
        {{true, true, false,true }, L"TLS obrigatorio; CA confiavel exigida", L"sem sessao em execucao", L"sem evidencia atual"},
        {{true, true, true, false}, L"TLS obrigatorio; CA confiavel exigida", L"em execucao", L"sem evidencia atual"},
        {{true, true, true, true }, L"TLS obrigatorio; CA confiavel exigida", L"em execucao", L"verificado"},
    };
    for (std::size_t i=0; i<sizeof(cases)/sizeof(cases[0]); ++i) {
        const auto& c=cases[i];
        const auto text=axv::format_native_security_status(c.state);
        for(const auto& required : {
                std::wstring(L"Politica SIP: ")+c.policy,
                std::wstring(L"Sessao nativa: ")+c.process,
                std::wstring(L"Registro SIP atual: ")+c.registration,
                std::wstring(L"Midia: este candidato nao oferece SRTP"),
                std::wstring(L"Homologacao real: nao concluida")}) {
            if(text.find(required)==text.npos) {
                std::cerr << "Missing truthful security detail in case " << i << '\n';
                return 1;
            }
        }
        // All output must be static diagnostic prose: never account, route,
        // certificate-path or credential-shaped values; never proof of TLS.
        for(const auto* forbidden : {L"sip:",L"sips:",L"@",L"password=",L"endpoint_id",
                L"127.0.0.1",L"C:\\",L"/tmp/",L"TLS ativo",L"TLS verificado",
                L"SRTP verificado",L"audio verificado",L"homologacao concluida"}) {
            if(text.find(forbidden)!=text.npos) return 2;
        }
        if(text.empty() || text.find(L'\0')!=text.npos) return 3;
        const bool has_registration=text.find(L"Registro SIP atual: verificado")!=text.npos;
        if(has_registration != (i==11 || i==15)) return 4;
        if(!c.state.policy_selected && text.find(L"TLS obrigatorio")!=text.npos) return 5;
    }
    for(auto state:{axv::SipStatus::Registered,axv::SipStatus::Calling,axv::SipStatus::Ringing,axv::SipStatus::InCall}) {
        axv::SipRuntimeObservation observed{axv::RuntimeProcess::Running,state,axv::SipEvidence::VerifiedNativeApi,7,0,0};
        if(!axv::native_security_status(true,true,observed).verified_registration)return 6;
        observed.process=axv::RuntimeProcess::Failed;
        if(axv::native_security_status(true,true,observed).verified_registration)return 7;
        observed.process=axv::RuntimeProcess::Running;observed.session=0;
        if(axv::native_security_status(true,true,observed).verified_registration)return 8;
    }
    return 0;
}
