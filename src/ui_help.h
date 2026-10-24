#pragma once

namespace axv {
// Offline help; no links, network access or claims of SIP service availability.
inline constexpr wchar_t kFrontendHelp[] =
    L"AXE Voice Desktop - laboratorio\n\n"
    L"Ctrl+1: Teclado    Ctrl+2: Ajustes    Ctrl+3: Audio\n"
    L"Ctrl+L: Focar destino    Ctrl+K: Teclado\n"
    L"Ctrl+M: MINI/MAX    Ctrl+T: Tema    Ctrl+P: Fixar\n"
    L"Ctrl+R: Atualizar dispositivos (apenas em Audio)\n"
    L"Rolagem e roda do mouse: acessar campos em telas menores.\n"
    L"Foco via Tab rola para exibir o controle selecionado.\n"
    L"Ctrl+G: menu MAIS - opcoes de escritorio\n"
    L"Ctrl+D: salvar favorito   Ctrl+F: recuperar favorito\n"
    L"Ctrl+H: destinos recentes desta sessao (nao chamadas)\n"
    L"Ctrl+Shift+P: ocultar/mostrar destino na tela\n"
    L"F3: Auditoria SIP local (NAO registra nem habilita chamadas)\n"    L"F4: Consultar existencia de credencial SIP no Windows (nao valida senha)\n"
    L"F5: Abrir Gerenciador de Credenciais do Windows\n"
    L"F6: Lista de pendencias reais para liberar chamadas SIP\n"
    L"F7: Remover credencial local SIP (exige confirmacao)\n"
    L"Ctrl+Shift+R: recuperar ultimo destino digitado\n"    L"MAIS: copiar/colar destino validado (clipboard compartilhado)\n"
    L"MAIS: selecionar destino, desfazer edicao, limpar destino + recentes\n"
    L"Comandos de destino nao realizam chamadas e respeitam modo privado.\n"

    L"MAIS: iniciar minimizado, remover favorito, diagnostico anonimo\n"
    L"Modo privado limpa recentes e pausa captura de destinos.\n"
    L"Na aba Audio: INVERTER VOZ/TOQUE troca as preferencias locais.\n"
    L"TESTAR VOZ / TESTAR TOQUE: tom local em cada saida. Sem SIP.\n"
    L"Durante o teste: PARAR VOZ / PARAR TOQUE interrompe o tom local.\n"
    L"Volume local de teste VOZ e TOQUE: 25/50/75/100%, padrao 75%.\n"
    L"Esc: Voltar ao teclado    F1: Esta ajuda\n"
    L"F2: Checklist local de prontidao\n\n"
    L"Os campos validam somente a sintaxe local.\n"
    L"Sem registro SIP real, nao e possivel fazer, receber\n"
    L"ou manter chamadas e nao existe audio de chamada.\n"
    L"Motor Baresip em execucao NAO comprova registro SIP.\n"
    L"Os botoes de chamada permanecem bloqueados.";
inline constexpr const wchar_t* frontend_help_text() { return kFrontendHelp; }
} // namespace axv
