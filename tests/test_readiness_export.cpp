#include "../src/readiness_export.h"
#include <string>
int main() {
    using namespace axv;
    auto empty = format_readiness_export({});
    if (empty.find(L"CHAMADAS BLOQUEADAS") == std::wstring::npos ||
        empty.find(L"NAO comprova") == std::wstring::npos) return 1;
    const auto full = format_readiness_export({true,true,true,true,true,true});
    if (full.find(L"GATES LOCAIS COMPLETOS") == std::wstring::npos ||
        full.find(L"homologacao de producao") == std::wstring::npos) return 2;
    for (unsigned i=0;i<64;++i) {
        CallReadiness flags{!!(i&1),!!(i&2),!!(i&4),
                            !!(i&8),!!(i&16),!!(i&32)};
        const auto text = format_readiness_export(flags);
        if (text.size()>8192 || text.find(L"accountsecret")!=std::wstring::npos ||
            text.find(L"pbx.example.org")!=std::wstring::npos ||
            text.find(L"sip:private")!=std::wstring::npos) return 3;
        if (!call_readiness_gates_pass(flags) &&
            text.find(L"CHAMADAS BLOQUEADAS")==std::wstring::npos) return 4;
    }
    return 0;
}
