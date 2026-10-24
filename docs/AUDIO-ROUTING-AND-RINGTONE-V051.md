# AXE Voice v0.51 — inverter rotas e testar toque na saída selecionada

## Objetivo

No frontend Windows, selecionar saídas distintas para toque e voz e
**inverter as preferências sem alterar o microfone**. Também é possível
pressionar **TESTAR TOQUE** e executar uma amostra audível curta e sintética
via WASAPI no endpoint indicado para *toque*, sob ação explícita.

## Funcionalidade da aba Áudio

Os botões no mesmo alinhamento são **ATUALIZAR**, **INVERTER** e
**TESTAR TOQUE**. A inversão altera apenas IDs efêmeros de saída em RAM.
O teste de toque reproduz somente no endpoint atualmente selecionado como
toque. Se o usuário inverter as saídas, uma *nova* pressão em TESTAR TOQUE
seleciona a nova saída de toque. Um teste já iniciado conserva o endpoint
capturado para aquela reprodução.

- A preferência de voz não muda durante TESTAR TOQUE.
- O teste não ativa microfone, registra conta SIP ou consulta credenciais.
- Se não existir uma saída ativa, o teste fica indisponível.
- Em `--ui-smoke` e `--ui-blackbox` não há reprodução de áudio.
- O worker de áudio não recebe HWND; a UI aguarda o encerramento antes de
  descartar recursos Win32.

## Validação

A PR integra o `audio_role_swap` e o `ringtone_preview` já testados
individualmente, acrescentando smoke UI verificando que ambos os botões
existem e o de teste fica desabilitado no modo de CI.

Executar MSVC Release, CTest completo e os gates `unit`, `blackbox`,
`integration` antes da incorporação.

## Limitações importantes

Este é um **teste de tom local**, não a campainha de uma chamada recebida.
Tampouco roteia a voz de chamadas SIP ou mídia RTP/SRTP para a saída
preferida. Para uso real faltam integração auditada no Baresip, drivers
de headset/PC, testes com PBX, TLS/SRTP e homologação física Windows 10/11.
A `main` permanece protegida e a PR segue Draft.
