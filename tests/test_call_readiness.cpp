#include "../src/call_readiness.h"
#include <string>
int main() {
    using namespace axv;
    const CallReadiness empty{};
    const std::wstring text = format_call_readiness(empty);
    if (call_readiness_gates_pass(empty) ||
        text.find(L"CHAMADAS BLOQUEADAS") == std::wstring::npos ||
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
    return 0;
}
