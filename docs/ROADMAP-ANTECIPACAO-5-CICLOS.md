# AXE Voice Desktop — Antecipação de cinco ciclos (outubro de 2026)

## Escopo e interpretação
A antecipação corresponde a entregas de software e de preparação técnica
executadas em cinco ciclos consecutivos. **Não são cinco meses de operação
real homologados**, nem prova de chamadas SIP/RTP. O frontend LAB mantém
`DisabledIncomingRingtoneSink` e `UnavailableSipEngine` até liberação
de um SDK Baresip auditado e teste em PABX e dispositivos Windows reais.

| Ciclo | Versão | Entrega antecipada | Evidência | Estado |
|---|---|---|---|---|
| Mês 1 | v0.57 | Worker cancelável de campainha, Stop ACK, thread única | CTest, PR #72 | Código entregue e CI aprovada |
| Mês 2 | v0.58 | Cancelamento cooperativo até a rotina WASAPI | CTest, PR #73 | Código entregue e CI aprovada |
| Mês 3 | v0.59 | Watchdog de ciclos de toque com falha fechada | CTest, PR #74 | Código entregue e CI aprovada |
| Mês 4 | v0.60 | Matriz determinística de 21.000 cenários de controle | CTest, PR #75 | Código entregue; conferir CI da respectiva PR |
| Mês 5 | v0.61 | Verificação PE/sha256 da build LAB e plano de homologação | CTest, documentação | Pendente de CI neste ciclo |

## Critérios que **não** foram satisfeitos
- Nenhuma captura e reprodução de voz SIP real (RTP/SRTP), áudio full duplex
  ou registro de conta SIP validado contra um PBX.
- A ligação opcional Baresip/libre, ABI, bibliotecas transitivas e hashes de
  produção ainda exige SDK auditado fornecido pelo operador.
- Nenhuma medição física de latência, eco, microfone, Bluetooth ou volume real.
- Nenhuma assinatura de produção, instalação comercial ou publicação da main.

## Gatilhos de GO operacional (todos obrigatórios)
1. Revisar e aprovar cadeia das PRs Draft; confirmar CI verde **no commit
   escolhido** e rodar revisão de segurança da ligação Baresip/WASAPI.
2. Fornecer SDK nativo Baresip/libre, documentação ABI e hashes aprovados.
3. Validar em máquina Windows 10/11 com PC e headset USB/Bluetooth: entrada
   /saída distintas, inversão voz/toque, parada e reconnect.
4. Executar PABX de testes: registro, chamada entrante/saínte,
   atendimento, hold, transferência, desligamento e falha da rede.
5. Capturar evidência de RTP/SRTP bidirecional, TLS, permissão do usuário,
   latência/jitter/packet loss e regras de firewall.
6. Validar rollback, instalador assinado, antivírus e integridade dos arquivos.
7. Registrar aceite formal de QA e segurança antes de mergear para `main`.

## Política de comunicação
'100%' refere-se **somente à taxa de aprovação da suíte executada**.
Não significa 100% de cobertura do código, cumprimento do roadmap,
qualidade acústica ou homologação com hardware real.
