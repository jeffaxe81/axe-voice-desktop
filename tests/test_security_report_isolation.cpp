#include "../src/diagnostic_export.h"
#include <array>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <vector>

// Compile-time boundary checks: the public report contains no account, device,
// destination, SDK diagnostic, credential, or arbitrary string input parameter.
static_assert(std::is_same_v<decltype(&axv::format_diagnostic_export),
    std::wstring (*)(const axv::CallReadiness&, const axv::NativeSecurityStatus&)>);
static_assert(std::is_aggregate_v<axv::CallReadiness>);
static_assert(std::is_aggregate_v<axv::NativeSecurityStatus>);
static_assert(!std::is_constructible_v<axv::CallReadiness, std::wstring>);
static_assert(!std::is_constructible_v<axv::NativeSecurityStatus, std::wstring>);

namespace {
bool boolean_members_only() {
    axv::CallReadiness r{};
    auto& [a,b,c,d,e,f,g]=r; // Also fails compilation if an input field is added.
    axv::NativeSecurityStatus s{};
    auto& [h,i,j,k]=s;
    return std::is_same_v<decltype(a),bool> && std::is_same_v<decltype(b),bool> &&
        std::is_same_v<decltype(c),bool> && std::is_same_v<decltype(d),bool> &&
        std::is_same_v<decltype(e),bool> && std::is_same_v<decltype(f),bool> &&
        std::is_same_v<decltype(g),bool> && std::is_same_v<decltype(h),bool> &&
        std::is_same_v<decltype(i),bool> && std::is_same_v<decltype(j),bool> &&
        std::is_same_v<decltype(k),bool>;
}
std::vector<std::wstring> lines(const std::wstring& text) {
    std::vector<std::wstring> result;
    std::size_t begin=0;
    while(begin<text.size()) {
        const auto end=text.find(L"\r\n",begin);
        result.push_back(text.substr(begin,end==text.npos?end:end-begin));
        if(end==text.npos)break;
        begin=end+2;
    }
    return result;
}
bool check_report(unsigned r,unsigned s) {
    const auto bit=[](unsigned value,unsigned n){return (value&(1u<<n))!=0;};
    const axv::CallReadiness ready{bit(r,0),bit(r,1),bit(r,2),bit(r,3),bit(r,4),bit(r,5),bit(r,6)};
    const axv::NativeSecurityStatus security{bit(s,0),bit(s,1),bit(s,2),bit(s,3)};
    const auto report=axv::format_diagnostic_export(ready,security);
    if(report.empty() || report.find(L'\0')!=report.npos)return false;
    for(std::size_t n=0;n<report.size();++n) {
        if(report[n]==L'\r' && (n+1==report.size() || report[n+1]!=L'\n'))return false;
        if(report[n]==L'\n' && (!n || report[n-1]!=L'\r'))return false;
    }
    // Exact full-line whitelist: no raw value or diagnostic suffix may be
    // appended to otherwise valid prose. Version is intentionally fixed here.
    const std::vector<std::wstring> expected{
        L"AXE Voice candidate 0.66.0",L"AXE VOICE - PRONTIDAO DE CHAMADAS",L"",
        bit(r,0)?L"[OK] Destino: sintaxe valida":L"[PENDENTE] Destino: corrigir ou informar destino",
        bit(r,1)?L"[OK] Conta: sintaxe local valida":L"[PENDENTE] Conta: revisar servidor, usuario e porta",
        bit(r,2)?L"[OK] Motor nativo: adaptador disponivel":L"[PENDENTE] Motor nativo: adaptador Baresip nao integrado",
        bit(r,3)?L"[OK] Processo: em execucao":L"[PENDENTE] Processo: nao esta em execucao",
        bit(r,4)?L"[OK] Registro SIP: evidencia nativa atual":L"[PENDENTE] Registro SIP: sem evidencia real de registro",
        bit(r,5)?L"[OK] Midia: caminho de audio verificado":L"[PENDENTE] Midia: RTP/SRTP nao verificado",L"",
        bit(r,6)?L"Botao Chamar: disponivel na interface; motor revalida ao discar.":L"Botao Chamar: indisponivel na interface.",L"",
        (r&63)==63?L"GATES LOCAIS COMPLETOS; homologacao real ainda necessaria.":L"CHECKLIST PENDENTE - homologacao nao concluida.",L"",
        L"Diagnostico local. Nao conecta ao SIP nem valida audio real.",L"",L"SEGURANCA DA SESSAO",
        !bit(s,0)?L"Politica SIP: sem politica de sessao selecionada":bit(s,1)?L"Politica SIP: TLS obrigatorio; CA confiavel exigida":L"Politica SIP: UDP sem criptografia",
        bit(s,2)?L"Sessao nativa: em execucao":L"Sessao nativa: sem sessao em execucao",
        bit(s,0)&&bit(s,2)&&bit(s,3)?L"Registro SIP atual: verificado":L"Registro SIP atual: sem evidencia atual",
        L"Midia: este candidato nao oferece SRTP",L"Homologacao real: nao concluida"};
    return lines(report)==expected;
}
}
int main() {
    if(!boolean_members_only())return 1;
    for(unsigned r=0;r<128;++r)for(unsigned s=0;s<16;++s)
        if(!check_report(r,s)) {std::cerr<<"Report isolation mismatch: "<<r<<", "<<s<<'\n';return 2;}
    // Proof freshness is independently checked against the caller's generation;
    // formatting consumes metadata only and must not invent registration proof.
    unsigned observations=0;
    for(auto process:{axv::RuntimeProcess::Stopped,axv::RuntimeProcess::Running,axv::RuntimeProcess::Stopping,axv::RuntimeProcess::Failed})
    for(auto evidence:{axv::SipEvidence::None,axv::SipEvidence::VerifiedNativeApi})
    for(auto status:{axv::SipStatus::Unavailable,axv::SipStatus::Disconnected,axv::SipStatus::Registering,axv::SipStatus::Registered,axv::SipStatus::Calling,axv::SipStatus::Ringing,axv::SipStatus::InCall,axv::SipStatus::Failed})
    for(unsigned session:{0u,7u,8u}) {
        const axv::SipRuntimeObservation view{process,status,evidence,session,17,16};
        const bool proof=process==axv::RuntimeProcess::Running && evidence==axv::SipEvidence::VerifiedNativeApi && session==7;
        if(axv::sip_proof_is_current(view,7)!=proof)return 3;
        if(!proof && axv::effective_sip_status(view,7)!=axv::SipStatus::Unavailable)return 4;
        const bool registered=status==axv::SipStatus::Registered || status==axv::SipStatus::Calling || status==axv::SipStatus::Ringing || status==axv::SipStatus::InCall;
        for(bool selected:{false,true})for(bool tls:{false,true}) {
            const auto security=axv::native_security_status(selected,tls,view);
            const bool own_proof=process==axv::RuntimeProcess::Running && evidence==axv::SipEvidence::VerifiedNativeApi && session!=0;
            if(security.verified_registration!=(selected && registered && own_proof))return 5;
            const auto report=axv::format_diagnostic_export({},security);
            const bool verified=report.find(L"Registro SIP atual: verificado")!=report.npos;
            if(verified!=(selected && registered && own_proof))return 6;
        }
        ++observations;
    }
    std::cout<<"2048 report combinations and "<<observations<<" proof observations verified.\n";
}
