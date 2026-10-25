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
    L"Roda do mouse e rolagem vertical em telas pequenas.\n\n"
    L"Os campos validam somente a sintaxe local.\n"
    L"Sem registro SIP real, nao e possivel fazer, receber\n"
    L"ou manter chamadas e nao existe audio de chamada.\n"
    L"Motor Baresip em execucao NAO comprova registro SIP.\n"
    L"Os botoes de chamada permanecem bloqueados.";
inline constexpr const wchar_t* frontend_help_text() {
#ifdef AXV_ENABLE_NATIVE_SIP
 return L"AXE Voice - candidato nativo\n\n"
        L"Em Ajustes, informe servidor, usuario, porta e senha da sessao.\n"
        L"Para TLS, selecione TLS e informe um arquivo CA PEM confiavel.\n"
        L"Clique REGISTRAR e aguarde SIP REGISTRADO.\n"
        L"No teclado, informe o ramal e use CHAMAR.\n"
        L"Para receber, use ATENDER; para terminar, ENCERRAR.\n"
        L"Pare o motor antes de trocar os dados da conta.\n\n"
        L"Chamadas usam o dispositivo padrao de comunicacao do Windows.\n"
        L"As selecoes na aba Audio sao apenas previas locais.\n"
        L"TLS protege a sinalizacao SIP; este candidato nao oferece SRTP.\n"
        L"Chamadas na central e audio fisico ainda exigem homologacao.\n\n"
        L"Ctrl+1: Teclado  Ctrl+2: Ajustes  Ctrl+3: Audio\n"
        L"F1: Ajuda  F2: Prontidao  Ctrl+M: MINI/MAX";
#else
 return kFrontendHelp;
#endif
}
} // namespace axv
