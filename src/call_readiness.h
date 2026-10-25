#pragma once
#include <string>
namespace axv {
// Local, privacy-preserving diagnostic only: no call authorization here.
struct CallReadiness {
    bool destination_syntax = false;
    bool account_syntax = false;
    bool native_engine = false;
    bool running_process = false;
    bool verified_registration = false;
    bool verified_media = false;
    bool dial_control_available = false; // UI gate only; engine revalidates commands.
};
inline bool call_readiness_gates_pass(const CallReadiness& s) {
    return s.destination_syntax && s.account_syntax && s.native_engine &&
           s.running_process && s.verified_registration && s.verified_media;
}
inline std::wstring format_call_readiness(const CallReadiness& s) {
    std::wstring out = L"AXE VOICE - PRONTIDAO DE CHAMADAS\n\n";
    auto line = [&out](const wchar_t* label, bool ok, const wchar_t* detail) {
        out += ok ? L"[OK] " : L"[PENDENTE] ";
        out += label; out += L": "; out += detail; out += L"\n";
    };
    line(L"Destino", s.destination_syntax, s.destination_syntax ?
        L"sintaxe valida" : L"corrigir ou informar destino");
    line(L"Conta", s.account_syntax, s.account_syntax ?
        L"sintaxe local valida" : L"revisar servidor, usuario e porta");
    line(L"Motor nativo", s.native_engine, s.native_engine ?
        L"adaptador disponivel" : L"adaptador Baresip nao integrado");
    line(L"Processo", s.running_process, s.running_process ?
        L"em execucao" : L"nao esta em execucao");
    line(L"Registro SIP", s.verified_registration, s.verified_registration ?
        L"evidencia nativa atual" : L"sem evidencia real de registro");
    line(L"Midia", s.verified_media, s.verified_media ?
        L"caminho de audio verificado" : L"RTP/SRTP nao verificado");
    out += s.dial_control_available ?
        L"\nBotao Chamar: disponivel na interface; motor revalida ao discar.\n" :
        L"\nBotao Chamar: indisponivel na interface.\n";
    out += call_readiness_gates_pass(s) ?
        L"\nGATES LOCAIS COMPLETOS; homologacao real ainda necessaria." :
        L"\nCHECKLIST PENDENTE - homologacao nao concluida.";
    out += L"\n\nDiagnostico local. Nao conecta ao SIP nem valida audio real.";
    return out;
}
} // namespace axv
