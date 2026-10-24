#pragma once

namespace axv {
// Offline help; no links, network access or claims of SIP service availability.
inline constexpr wchar_t kFrontendHelp[] =
    L"AXE Voice Desktop - laboratorio\n\n"
    L"Ctrl+1: Teclado    Ctrl+2: Ajustes    Ctrl+3: Audio\n"
    L"Ctrl+L: Focar destino    Ctrl+K: Teclado\n"
    L"Ctrl+M: MINI/MAX    Ctrl+T: Tema    Ctrl+P: Fixar\n"
    L"Ctrl+R: Atualizar dispositivos (apenas em Audio)\n"
    L"Esc: Voltar ao teclado    F1: Esta ajuda\n"
    L"F2: Checklist local de prontidao\n"
    L"Ctrl+Shift+D: Copiar status tecnico sem dados pessoais\n\n"
    L"Os campos validam somente a sintaxe local.\n"
    L"Sem registro SIP real, nao e possivel fazer, receber\n"
    L"ou manter chamadas e nao existe audio de chamada.\n"
    L"Motor Baresip em execucao NAO comprova registro SIP.\n"
    L"Os botoes de chamada permanecem bloqueados.";
inline constexpr const wchar_t* frontend_help_text() { return kFrontendHelp; }
} // namespace axv
