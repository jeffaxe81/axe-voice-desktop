# AXE Voice LAB — v0.150–v0.154: diagnóstico assíncrono na aba Ajustes

**Data:** 25/10/2026. **Sem executar compilação, testes ou homologação.**

## Cinco entregas incrementais

| Versão | Alteração efetiva |
| --- | --- |
| 0.150 | Porta o restante da PR histórica [#30](https://github.com/jeffaxe81/axe-voice-desktop/pull/30) no botão **Dispositivos de áudio** de Ajustes: sai a chamada síncrona `inspect_windows_audio_devices()` na thread GUI, entra `AudioInventoryAsync`. |
| 0.151 | Introduz `g_settings_audio_worker` e `g_settings_audio_retry` independentes dos equivalentes da aba Áudio; uma aba não consome o resultado da outra. |
| 0.152 | Ao sair de Ajustes, descarta a geração vigente e cancela a pendência. Novo clique durante uma consulta invalida o snapshot anterior e mantém uma única atualização subsequente. |
| 0.153 | O timer da GUI retoma uma consulta pendente quando o worker estiver livre e exibe resultados apenas na aba Ajustes visível, sem acessar `HWND` pela thread auxiliar. |
| 0.154 | Atualiza CMake, janela de laboratório, diagnóstico e README. Evita reimplementar o checklist anônimo, que já tinha sido integrado pela [PR #119](https://github.com/jeffaxe81/axe-voice-desktop/pull/119) (a PR antiga #117 foi fechada sem merge). |

## Limites importantes

- É uma reconciliação **parcial** de um rascunho histórico. Não encerra nem mescla integralmente a PR #30; seu código de 2026 foi adaptado a partir da base atual v0.149.
- As consultas usam threads desacopladas com estado compartilhado e invocam a inspeção que inicializa a COM na própria thread de execução. A aplicação dos resultados ocorre exclusivamente no Win32 UI timer.
- A alteração não ativa conta Baresip, SIP REGISTER, TLS, codecs, RTP, SRTP, ringtone de chamadas ou áudio bidirecional. `UnavailableSipEngine` e `SipEvidence::None` permanecem.
- Ainda são necessários compilação MSVC/Windows, navegação repetida, cliques enquanto consultas anteriores estiverem ocupadas, saída da aplicação durante bloqueio do driver, cenários de COM indisponível, memória e UI acessível. **Nenhum desses testes foi executado nesta rodada.**
- A branch `main` preserva seu legado MicroSIP/PJSIP; a promoção de código nativo continua pendente da homologação.

Esta documentação permite rastrear o merge sem tratá-lo como aceite operacional.
