# AXE Voice LAB — v0.131–v0.134: reconciliação seletiva de rascunhos antigos

Data: 24/10/2026. **Não houve compilação nem execução de testes nesta rodada**, conforme solicitado.

## Conteúdo portado para código atual

| Release LAB | Fonte histórica | Mudança efetiva |
|---|---|---|
| v0.131 | [PR #39](https://github.com/jeffaxe81/axe-voice-desktop/pull/39) | `SipEventProjection::observe` rejeita lote com `Batch.size > Batch.events.size()`, antes de ler eventos; a geração é invalidada. |
| v0.132 | [PR #18](https://github.com/jeffaxe81/axe-voice-desktop/pull/18) | Falha temporária de `inspect_windows_audio_devices()` limpa/desabilita controles de dispositivo e interrompe prévias, porém não apaga IDs selecionados. |
| v0.133 | [PR #16](https://github.com/jeffaxe81/axe-voice-desktop/pull/16) | Microfone, saída de voz e saída de toque têm IDs de **preferência explícita em memória**, separados do endpoint de fallback. Atualizações automáticas não alteram preferências, e um endpoint reconectado volta a ser selecionado. Troca manual entre voz/toque atualiza ambas preferências. |
| v0.134 | Reconciliação | Versão `CMakeLists.txt`, diagnóstico LAB e janela Win32 atualizada para `0.134.0`. README distingue correção portável de PR antiga completa. |

## Sem equivalência presumida

**Estas três PRs históricas NÃO estão integralmente mescladas ou fechadas.** Elas também contêm testes, documentação antiga, alterações de compilação ou outras funções divergentes; nenhum desses itens foi incorporado automaticamente. A PR #36 contém uma `ui_viewport_policy.h` byte a byte equivalente na base atual, inspecionada na triagem; não há benefício em reaplicar esse arquivo.

## Limitações e requisitos pendentes

- Código não compilado, sem CTest/CI manual ou testes reais de dispositivos.
- PABX, registro TLS/SIP, RTP/SRTP e áudio de chamada ainda não homologados; o executável usa `UnavailableSipEngine` e `SipEvidence::None`.
- Caso a tentativa de consulta de dispositivo falhe, os controles ficam indisponíveis enquanto a seleção anterior permanece em memória, não declarada como um dispositivo online.
- Fallback só funciona após nova enumeração bem-sucedida. Não há persistência de preferências em registro ou arquivo.
- É necessário realizar teste posterior de reconexão/remoção real de headset, UI do Windows, leitura de arquivo de áudio e compilação MSVC antes de promoção do motor para `main`.
- A `main` mantém a arquitetura histórica MicroSIP/PJSIP e não recebe este código experimental nesta etapa.
