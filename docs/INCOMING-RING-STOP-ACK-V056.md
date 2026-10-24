# AXE Voice v0.56 — Stop ACK para futura campainha de chamadas entrantes

## Objetivo

Integrar a decisão de campainha da v0.55 com um **coordenador**
que impede começar a tocar em um novo endpoint Windows (headset/PC)
até que o reprodutor anterior confirme que parou completamente.

## Contrato

O `IncomingRingtoneCoordinator` só considera `Start` quando
`IncomingRingtoneGate` comprova evento nativo, sessão SIP válida,
chamada recebida ativa, token opaco atual e saída de toque presente.

Quando houver troca de dispositivo, atendimento, desligamento,
desconexão, perda da prova nativa ou cancelamento manual de rota:

- Emite `Stop` ao futuro `IncomingRingtoneSink`;
- Se `stop()` retornar **false**, entra em `StopPending`, sem admitir
  a nova saída, e tenta novamente na próxima atualização da UI;
- Somente após `stop()` confirmar silêncio retorna `Stopped`.
  **Nova autorização Start exige avaliação posterior**;
- `start()` pode falhar após ativação parcial: exige `stop()`
  preventivo e não tenta repetidamente a mesma sessão/chamada.

## Estado do executável

O frontend possui `DisabledIncomingRingtoneSink`, que nunca toca
campainha. O observador de sessão Baresip ainda fornece
`SipEvidence::None`, de forma que o gate não pode emitir Start.
Não existe player automático de chamadas conectado. **TESTAR VOZ** e
**TESTAR TOQUE** são controles de tons locais independentes que não
participam deste contrato.

## Qualidade

- `incoming_ringtone_coordinator`: Stop pendente/ACK, troca PC-headset,
  falha ao iniciar, recusa de retry, revogação e backend desativado.
- `integration_ringtone_playback_control`: mailbox sintética →
  projeção SIP → coordenador → sink simulado; Start/Stop sem duplicidade.
- Regressões: 3 gates CTest Windows; UI smoke não pode ativar toque ou
  discagem automática.

Para ativar uma campainha verdadeira ainda faltam SDK nativo Baresip
auditado, confirmação de registro/call-event real, sink WASAPI próprio
com stop verificável, homologação Windows 10/11/PBX/headsets e TLS/SRTP.
Manter PR Draft até aprovação, sem merge na main.
