# AXE Voice LAB — bloquear retrocesso de geração SIP (NÃO TESTADO)

## Problema e mudança

A projeção `SipEventProjection` rejeitava uma geração explicitamente bloqueada, mas não mantinha o maior número de sessão que já havia observado. Em uma sequência anormal de batches de callbacks, uma geração mais antiga poderia voltar a ser considerada após o início de outra sessão, se o produtor de eventos informasse evidência nativa como se estivesse válida.

A projeção agora preserva `highest_session_`, um teto **monotônico** por instância:

- Um batch com `session` maior amplia o teto e segue pelas validações existentes.
- Um batch com `session` menor **não é processado**: o projetor falha fechado, revoga a geração de maior número e exige uma nova geração posterior.
- Um batch com a mesma geração continua sujeito às barreiras de prova, buffer, sessão rejeitada e ordem dos eventos preexistentes.
- A interrupção do processo não diminui o teto: nenhum callback de uma sessão antiga pode ressuscitar o registro após uma parada/reinicialização.

A mudança não autentica eventos por si só e **não altera** os limites de prova `SipEvidence::VerifiedNativeApi` ou a segurança do SDK experimental.

## Situação e pendências

Mudança integrada somente no laboratório. **Nenhum build MSVC, teste unitário, CTest, CI manual ou teste com PABX foi executado nesta rodada.** A GUI segue com `UnavailableSipEngine`, sem registro autenticado SIP, chamadas nem RTP/SRTP. Verificar futuramente batches atrasados, overflow, revogação, geração máxima e a política de monotonicidade de um mailbox único por ciclo de vida do aplicativo.

Código da PR #39 continua com bateria de testes histórica pendente; a alteração de segurança acima não equivale à execução desses testes.
