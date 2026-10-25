#include "../src/call_readiness.h"
#include <string>
int main() {
    using namespace axv;
    const CallReadiness registered_without_media{true,true,true,true,true,false};
    if(format_call_readiness(registered_without_media).find(L"CHAMADAS BLOQUEADAS") != std::wstring::npos) return 5;
    const CallReadiness empty{};
    const std::wstring text = format_call_readiness(empty);
    if (call_readiness_gates_pass(empty) ||
        text.find(L"CHECKLIST PENDENTE") == std::wstring::npos ||
        text.find(L"sem evidencia real de registro") == std::wstring::npos) return 1;
    const CallReadiness full{true,true,true,true,true,true};
    if (!call_readiness_gates_pass(full)) return 2;
    for (int i=0; i<6; ++i) {
        auto s=full;
        switch (i) {
        case 0: s.destination_syntax=false; break;
        case 1: s.account_syntax=false; break;
        case 2: s.native_engine=false; break;
        case 3: s.running_process=false; break;
        case 4: s.verified_registration=false; break;
        case 5: s.verified_media=false; break;
        }
        if (call_readiness_gates_pass(s)) return 3;
    }
    if (format_call_readiness(full).find(L"homologacao real ainda necessaria")
        == std::wstring::npos) return 4;
    auto ready_to_dial=registered_without_media;
    ready_to_dial.dial_control_available=true;
    const auto ready_text=format_call_readiness(ready_to_dial);
    if(ready_text.find(L"Botao Chamar: disponivel na interface") == std::wstring::npos ||
       ready_text.find(L"RTP/SRTP nao verificado") == std::wstring::npos ||
       ready_text.find(L"CHECKLIST PENDENTE") == std::wstring::npos ||
       call_readiness_gates_pass(ready_to_dial)) return 6;
    if(text.find(L"Botao Chamar: indisponivel na interface") == std::wstring::npos) return 7;
    // Completing the acceptance checklist alone never enables the dial control.
    if(format_call_readiness(full).find(L"Botao Chamar: indisponivel na interface") == std::wstring::npos) return 8;
    return 0;
}
