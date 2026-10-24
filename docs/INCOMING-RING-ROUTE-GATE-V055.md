# AXE Voice v0.55 — admissão de campainha ligada à sessão SIP e saída Windows

## Integração experimental

A UI Windows passa a consultar `IncomingRingtoneGate` utilizando o
estado observado do motor e o **identificador corrente em memória** da
saída selecionada para toque.

O gate emite decisões `Start`, `Stop` ou `None`. `Start` só é admitido
quando **todos** os critérios estão presentes:

- Observação produzida por API nativa comprovada (`VerifiedNativeApi`);
- Processo em execução, sessão atual e token de chamada não nulo;
- Estado `Ringing` de **chamada recebida**, não originada;
- Endpoint atual de toque presente no inventário e com ID igual à
  preferência selecionada.

Quando a chamada é atendida, encerrada, a sessão/registro perde validade,
o dispositivo desaparece ou a saída de toque muda, o gate revoga a
autorização anterior. **A inversão PC/headset revoga primeiro**, nunca
autoriza automaticamente reprodução na saída antiga.

## Limite essencial do executável

A execução de laboratório ainda observa `SipEvidence::None`. O
`IncomingRingtoneGate` é apenas um **gate de decisão**, não inicia o
reprodutor WASAPI nem aciona uma campainha ao receber chamadas. Os
botões **TESTAR VOZ**/**TESTAR TOQUE** continuam funções locais manuais.
Não existe ainda vínculo auditado de Baresip SDK nativo à interface,
registro SIP real, RTP/SRTP ou áudio bidirecional.

## QA

- `incoming_ringtone_gate` testa chamadas recebidas/originadas,
  mudanças de sessão, indisponibilidade do endpoint, alternância da saída,
  perda de prova e `Stop` antes de um novo `Start`.
- `integration_incoming_ringtone` usa exclusivamente mailbox e eventos
  nativos **sintéticos** para simular estabelecimento/fechamento da
  chamada, troca de rota, overflow e revogação.
- `ui_smoke` exige que nenhuma campainha fique admitida no LAB real,
  mantendo discagem bloqueada.
- Três gates CTest: unitário, caixa-preta e integração simulada.

Para habilitar toque real de chamadas recebidas falta uma bridge
certificada do SDK e player cancelável por evento, com teste físico em
Windows 10/11 e PABX. A PR permanece Draft e não modifica `main`.
