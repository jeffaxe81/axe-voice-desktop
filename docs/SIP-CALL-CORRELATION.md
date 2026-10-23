# AXE Voice — correlação dos eventos de chamada (laboratório)

Esta etapa endurece a política do frontend antes de conectar um Baresip nativo.
**Não existe registro SIP, chamada real nem fluxo de áudio implementado por esta mudança.**

## Contrato que o futuro adaptador nativo deve atender

- O adaptador em processo (não logs, stdout ou IPC desprotegido) comprova a
  procedência e a geração da sessão antes de conceder `VerifiedNativeApi`.
- Toda chamada ganha um identificador inteiro opaco, não zero e **estritamente
  crescente dentro de uma geração**. O adaptador deve associar cada callback à
  identidade da chamada e converter essa identidade para esse identificador.
- `CallIncoming` e `CallOutgoing` iniciam uma chamada com um ID novo.
  `CallEstablished` e `CallClosed` precisam do **mesmo ID**.
- Eventos de registro, desconexão e transporte devem usar `call_id = 0`.
  Número telefônico, URI SIP, ponteiros brutos e credenciais nunca entram na fila.
- Um callback atrasado de outra chamada, ID zero ou reutilização de identificador
  resulta em falha fechada da projeção da sessão, sem alterar a chamada errada.
- Após `TransportLost`, o identificador ativo é revogado. O valor mais alto
  permanece guardado até nova geração, para impedir reutilização acidental.
- Uma nova geração de sessão zera os identificadores; eventos da sessão antiga
  são rejeitados pela `BaresipEventMailbox` e pela projeção.

## Testes automatizados

`baresip_event_reducer` cobre transições válidas com identificação de chamadas.
`sip_event_projection` mantém a projeção orientada a eventos.
`baresip_call_correlation` cobre ID ausente, uma chamada encerrada antes da
seguinte, `CallClosed` tardio, reutilização de ID, perda de transporte e
bloqueio persistente até nova sessão.

São **testes de política com eventos sintéticos**. Não equivalem a homologação
de registro SIP, PABX, headset, RTP/SRTP ou chamada real.

## Próxima implementação necessária

1. Fixar versão auditada de Baresip/re e revisar licenças/artefatos.
2. Construir adaptador C/C++ com callbacks `bevent` reais, mapeamento estável
   de identidade de chamada para token monotônico e gestão de geração.
3. Integrar credenciais sem vazamentos, autenticação de conta, TLS e SRTP.
4. Exercitar ligação interna/externa, atender, rejeitar, desligar, áudio de
   ida e volta, hotplug e falhas/reconexão em Windows 10/11.
5. Manter `UnavailableSipEngine` e a PR Draft até essas etapas estarem
   comprovadas por teste e homologação real.

## v0.45 — Autorizações de comandos vinculadas à identidade da chamada

`src/call_command_authorization.h` complementa `authorize_dial` com o
contrato de `Answer` e `Hangup`. Exige prova de sessão nativa atual,
motor Baresip integrado, estado do motor coincidente com o estado da projeção
e ID de chamada idêntico ao identificador ativo e último ID observado.

- `Dial`: permitido somente sem chamada ativa e com destino normalizado;
  não aceita ID de chamada para a ação de originar.
- `Answer`: requer estado `Ringing`, token opaco não zero e sessão correta.
- `Hangup`: requer `Ringing`, `Calling` ou `InCall` e o mesmo token ativo.
- Botões e mensagens do protótipo não são prova de permissão. **O backend deverá
  repetir esta verificação no mesmo contexto/worker que envia o comando à API
  nativa**, pois uma chamada pode ser encerrada entre clique e execução.

`call_command_authorization` integra redutor, projeção e autorização com
eventos exclusivamente sintéticos, incluindo 100 IDs obsoletos. O app ainda
não dispõe do método nativo de atendimento/hangup nem de mídia; nenhum comando
real foi habilitado.
