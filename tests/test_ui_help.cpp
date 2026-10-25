#include "../src/ui_help.h"
#include <string_view>
int main() {
    const std::wstring_view text(axv::frontend_help_text());
    if (text.find(L"Ctrl+1") == text.npos || text.find(L"Ctrl+2") == text.npos ||
        text.find(L"Ctrl+3") == text.npos) return 1;
    if (text.find(L"F1: Esta ajuda") == text.npos ||
        text.find(L"Esc: Voltar") == text.npos) return 2;
    if (text.find(L"Sem registro SIP real") == text.npos ||
        text.find(L"NAO comprova registro SIP") == text.npos) return 3;
    if (text.find(L"Ctrl+R: Atualizar") == text.npos) return 4;
    return 0;
}
