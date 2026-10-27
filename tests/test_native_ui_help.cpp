#define AXV_ENABLE_NATIVE_SIP
#include "../src/ui_help.h"
#include <string_view>
int main(){
 const std::wstring_view help=axv::frontend_help_text();
 for (const auto clause : {L"consulta assincrona", L"Audio e Ajustes",
                           L"DISPOSITIVOS DE AUDIO", L"Pedidos repetidos sao agrupados",
                           L"Sair da aba cancela a atualizacao visual",
                           L"Falhas preservam as escolhas", L"reconsulta pode recuperar",
                           L"nao cancela a consulta no driver",
                           L"A rota fica fixa na sessao", L"preflight real no motor",
                           L"Inventario nao comprova audio"}) {
   if (help.find(clause) == help.npos) return 2;
 }
    // Removing a hotplug policy or its limitation must make help incomplete.
    for (const auto policy : {L"apenas com a aba Audio visivel", L"Eventos agrupados",
                               L"dropdown fechado", L"observador indisponivel",
                               L"consulta periodica", L"botao ATUALIZAR",
                               L"Notificacoes nao alteram a chamada",
                               L"Ajustes continua", L"diagnostico manual independente"}) {
        if (help.find(policy) == help.npos) return 3;
    }
 return help.find(L"REGISTRAR")!=help.npos && help.find(L"CA PEM")!=help.npos &&
        help.find(L"padrao de comunicacao")!=help.npos &&
        help.find(L"primeira observacao de chamada estabelecida")!=help.npos &&
        help.find(L"Inclui espera; nao mede qualidade nem audio")!=help.npos &&
        help.find(L"permanecem bloqueados")==help.npos ? 0:1;
}
