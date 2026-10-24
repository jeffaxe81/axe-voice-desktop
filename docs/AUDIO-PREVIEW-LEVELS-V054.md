# AXE Voice v0.54 — volumes independentes dos testes locais

## Interface de Áudio

A nova v0.54 consolida **INVERTER VOZ/TOQUE**, **TESTAR VOZ**,
**TESTAR TOQUE**, **PARAR VOZ/TOQUE** e dois controles de amplitude
independentes. Cada função aceita **25%, 50%, 75% ou 100%**;
o valor inicial é **75%** do *tom local de baixa amplitude*.

- `VOLUME TESTE - VOZ` acompanha o papel **voz**, mesmo quando a
  preferência de dispositivo muda de computador para headset.
- `VOLUME TESTE - TOQUE` acompanha o papel **toque**, independentemente
  da saída selecionada para a conversa.
- O percentual reduz a amplitude do tom gerado (máximo ~11% da escala
  digital a 100%); **não corresponde ao volume efetivo percebido**,
  não altera volume geral do Windows, microfone ou mídia telefônica.
- O worker captura **endpoint, papel e percentual no momento do clique**.
  Alterar o seletor de volume durante reprodução modifica apenas o
  próximo teste; trocar endpoint, inverter ou fechar solicita
  cancelamento cooperativo. A UI não reproduz áudio em `--ui-smoke`.

## Segurança e verificação

- PCM16 e float32 recebem a mesma política de ganho com testes
  determinísticos de amplitude, silêncio, valores inválidos e limites.
- CTest `audio_preview_levels` cobre seleção dos níveis e isolamento
  lógico entre volumes de voz/toque.
- `ringtone_preview` testa amostras dos dois formatos em memória,
  parâmetros rejeitados e cancelamento prévio ao acesso ao Windows.
- Smoke de interface verifica dois controles reais Win32 e as escolhas
  25% para voz / 100% para toque **sem tocar som**, sem autorizar chamadas.

## Restrições

**Somente laboratório:** os testes emitem tons sintéticos locais após
clique explícito, não a voz da ligação nem uma campainha acionada por SIP.
Roteamento de mídia Baresip, registro SIP, chamadas reais e RTP/SRTP
dependem de integração auditada e de homologação física Windows 10/11,
headset USB/Bluetooth e alto-falantes. PR Draft, sem merge em `main`.
