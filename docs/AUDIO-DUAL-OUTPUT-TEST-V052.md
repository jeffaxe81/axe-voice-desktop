# AXE Voice v0.52 — Testes separados de VOZ e TOQUE

## Entrega de frontend Windows

A aba Áudio oferece dois controles independentes:
**TESTAR VOZ** usa a saída escolhida para *voz da conversa*;
**TESTAR TOQUE** usa a saída escolhida para *toque*. Ambos reproduzem
o mesmo tom sintético curto e de baixa amplitude da implementação WASAPI
v0.51, por **clique explícito** e fora da thread gráfica.

O botão **INVERTER VOZ/TOQUE** troca apenas as preferências na memória.
Assim, os testes seguintes correspondem aos endpoints que aparecem
nos dois seletores. O microfone é preservado.

## Segurança e comportamento

- Um único worker de reprodução: um teste nunca é executado em paralelo
  com o outro e os dois botões ficam desabilitados durante o teste.
- Endpoint inválido ou removido bloqueia somente o teste desse papel,
  **sem fallback silencioso** para o outro dispositivo.
- Saídas iguais são aceitas e testadas individualmente, sem acesso
  simultâneo ao dispositivo.
- O endpoint é capturado no clique; mudar a preferência posteriormente
  não altera o teste já iniciado.
- Modo `--ui-smoke` ou `--ui-blackbox` não abre áudio, inclusive
  diante de comandos Win32 enviados diretamente aos controles.
- Nenhum ID de endpoint vai a logs, disco, conta SIP ou telemetria.

## Testes e limitações

`audio_preview_target` cobre seleção correta, ida/volta da inversão,
saídas iguais, saída desconectada, endpoint ausente e limites de ID.
`ui_smoke` verifica os dois botões, os dois destinos depois da
inversão e a ausência de worker ou autorização de chamada.

Os botões emitem **tons locais**, não a voz de uma chamada SIP e nem
a campainha recebida de um PBX. Não há roteamento de mídia Baresip,
registro SIP real, RTP/SRTP ou teste de headset físico automatizado.
É necessária homologação auditiva Windows 10/11 com speakers,
USB/Bluetooth, DPI alto e volumes apropriados antes de produção.
