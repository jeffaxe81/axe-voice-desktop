# AXE Voice v0.63 — Fence para alterações de dispositivos durante teste local

Base: v0.62 (PR #77), sem merge na `main`.

## Problema e correção
Antes, clicar em **INVERTER VOZ/TOQUE**, **ATUALIZAR** ou trocar um dispositivo durante um tom WASAPI podia atualizar a seleção antes da confirmação de parada do worker. Isso não habilitava SIP, mas dava uma indicação enganosa durante a reprodução.

Agora o frontend:
- Solicita cancelamento cooperativo e mantém a seleção em uso até o worker encerrar.
- Aguarda `join()` no ciclo de UI antes de aplicar a última alteração solicitada (swap, refresh ou dispositivo de microfone/voz/toque).
- Restaura visualmente a seleção anterior no combo enquanto aguarda a parada.
- Revalida o **ID** em memória do dispositivo no momento da aplicação; jamais reaproveita índice antigo.
- Mantém volumes independentes, teste local e bloqueio de SIP. Encerrar a janela limpa mudanças pendentes.

**Limites:** é uma garantia de sequenciamento de testes *locais*; o LAB continua com `UnavailableSipEngine` e `DisabledIncomingRingtoneSink`. Não valida Baresip real, mídia RTP/SRTP, chamada SIP ou funcionamento de headset físico.

## Testes
`audio_route_change_fence` valida cancelamento e confirmação, última intenção, limpeza e 10.000 interações simuladas; a CI Windows deverá validar também os smoke tests e o restante da regressão.

## Critérios de aceitação pendentes
- [ ] CI MSVC / CTest verde e pacote LAB consistente.
- [ ] Ensaios físicos com PC, headset USB e Bluetooth, inclusive desconexão durante tom.
- [ ] Integração auditada do motor SIP nativo, sem ativação neste incremento.
