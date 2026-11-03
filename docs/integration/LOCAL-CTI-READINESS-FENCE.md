# AXE Voice v0.83 — barreira de prontidão SIP para CTI local

## Problema e proteção

Uma solicitação CTI autenticada pode receber HTTP 202 (`queued`) quando
a conta SIP está pronta e perder essa condição antes da discagem na GUI.
A candidata v0.83 **rejeita o pedido ainda enfileirado** quando a
prontidão desaparece, preservando o histórico limitado e publicando
a transição `rejected` para consultas autenticadas.

`LocalCtiServer::take()` verifica a prontidão antes de retirar um pedido
e confere novamente após a transição para `dispatching`. Caso haja
perda nessa segunda janela, registra `rejected` e não entrega a
solicitação ao chamador. A GUI ainda deve revalidar a sessão SIP antes
de executar `start_call()`. Não se promete cancelar uma chamada já
enviada ao motor SIP.

## Comportamento para integrações

1. `POST /v1/calls` devolve `202` e `request_id` local:
   não representa chamada estabelecida.
2. Se SIP perder prontidão enquanto o pedido estiver na fila,
   `GET /v1/requests/{id}` passa a devolver `rejected`.
3. `GET /v1/requests/{id}/events` inclui `queued` e
   `rejected`, sem expor número discado ou segredos.
4. Retry com a mesma `Idempotency-Key` e destino retorna `200`
   com `replayed:true` e `status:rejected`, sem nova discagem
   depois que o SIP recuperar. Uma nova intenção precisa de outra chave.
5. Mesma chave com outro destino continua devolvendo
   `409 idempotency_key_conflict`.

`GET /v1/capabilities` agora declara
`readiness_loss_rejects_queued:true`. A interface permanece restrita
ao loopback e autenticada por Bearer token; não habilita CORS ou webhooks.

## Cobertura e limites

Testes unitários verificam rejeição atômica, evento terminal único,
replay após perda SIP e preservação de `dispatching`. Testes HTTP
loopback verificam a perda após `202`, o resultado `rejected`,
eventos, retry sem rediscagem e pedido novo após recuperação.

Exigir CI Windows, testes do SDK nativo, bateria CTI e instalador
no SHA final antes de merge. A alteração não comprova conexão real
à central Dígitro, áudio bidirecional com headset ou SIP/RTP/SRTP.
Idempotência permanece limitada ao histórico do processo (32
resultados); reinício apaga as chaves, e o sistema não oferece
garantia de exactly-once entre reinícios.
