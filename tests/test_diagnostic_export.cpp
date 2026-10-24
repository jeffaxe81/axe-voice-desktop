#include "../src/diagnostic_export.h"
#include <string>
int main() {
    using namespace axv;
    const auto blocked = format_diagnostic_export(CallReadiness{});
    if (blocked.find(L"CHAMADAS BLOQUEADAS") == std::wstring::npos ||
        blocked.find(L"Somente estados") == std::wstring::npos ||
        blocked.find(L"sem evidencia real de registro") == std::wstring::npos) return 1;
    for (std::size_t i=0; i<blocked.size(); ++i) {
        if (blocked[i] == L'\n' && (i==0 || blocked[i-1] != L'\r')) return 2;
        if (blocked[i] == L'\r' && (i+1>=blocked.size() || blocked[i+1]!=L'\n')) return 3;
    }
    const auto ready = format_diagnostic_export(CallReadiness{true,true,true,true,true,true});
    if (ready.find(L"GATES LOCAIS COMPLETOS") == std::wstring::npos ||
        ready.find(L"homologacao real ainda necessaria") == std::wstring::npos) return 4;
    const auto partial = format_diagnostic_export(CallReadiness{true,true,true,true,false,true});
    if (partial.find(L"CHAMADAS BLOQUEADAS") == std::wstring::npos ||
        partial.find(L"sem evidencia real de registro") == std::wstring::npos) return 5;
    // The formatter's type accepts no raw identifiers or credentials.
    if (blocked.find(L"sip:1001@example.org") != std::wstring::npos ||
        blocked.find(L"senha=") != std::wstring::npos) return 6;
    return 0;
}
