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
    // The help must explain what users observe during asynchronous inventory.
    for (const auto clause : {L"consulta assincrona", L"Audio e Ajustes",
                              L"Pedidos repetidos sao agrupados",
                              L"Sair da aba cancela a atualizacao visual",
                              L"Falhas preservam as escolhas", L"reconsulta pode recuperar",
                              L"nao cancela a consulta no driver",
                              L"Inventario nao comprova audio"}) {
        if (text.find(clause) == text.npos) return 5;
    }
    // Removing a hotplug policy or its limitation must make help incomplete.
    for (const auto policy : {L"apenas com a aba Audio visivel", L"Eventos agrupados",
                               L"dropdown fechado", L"observador indisponivel",
                               L"consulta periodica", L"botao ATUALIZAR",
                               L"Notificacoes nao alteram a chamada",
                               L"Ajustes continua", L"diagnostico manual independente"}) {
        if (text.find(policy) == text.npos) return 6;
    }
    return 0;
}
