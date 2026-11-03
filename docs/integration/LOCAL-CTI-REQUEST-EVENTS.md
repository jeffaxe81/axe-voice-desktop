# AXE Voice v0.77 — eventos CTI correlacionados por solicitação

A versão 0.77 acrescenta consulta aos **eventos de execução de cada
solicitação CTI**, associados ao `request_id`. Esses registros descrevem
apenas o processamento local do pedido, **não comprovam chamada atendida**.

## GET /v1/request-events?after=0

Base URL: `http://127.0.0.1:27652`
Autorização: `Authorization: Bearer <token>`
O parâmetro `after` é opcional e indica a última sequência recebida.

```json
{
  "latest_sequence": 3,
  "oldest_sequence": 1,
  "gap": false,
  "events": [
    {"sequence": 1, "request_id": 15, "status": "queued"},
    {"sequence": 2, "request_id": 15, "status": "dispatching"},
    {"sequence": 3, "request_id": 15, "status": "submitted"}
  ]
}
```

Possíveis estados: `queued`, `dispatching`, `submitted`, `rejected` e
`cancelled`. O status `submitted` prova somente que o motor aceitou a
solicitação local; não confirma INVITE, resposta SIP, mídia RTP/SRTP,
atendimento ou encerramento da chamada.

## Exemplo PowerShell

```powershell
$headers = @{ Authorization = "Bearer $env:AXV_LOCAL_API_TOKEN" }
$page = Invoke-RestMethod -Uri "http://127.0.0.1:27652/v1/request-events?after=0" -Headers $headers
$page.events | Where-Object { $_.request_id -eq 15 }
```

Para processar o próximo lote, use o `latest_sequence` recebido como
novo `after`. Se `gap` for verdadeiro, parte do histórico foi descartada:
reconcilie consultando `GET /v1/requests/{id}` e
`GET /v1/calls/current`.

## Regras de segurança e limites

- Histórico em memória limitado às **64 transições mais recentes**.
  Os **32 resultados de solicitação** existentes permanecem separados.
- O envio, retirada, finalização e cancelamento de pedidos registram
  suas transições de forma atômica sob o mutex da fila.
- Reenvio com a mesma chave `Idempotency-Key` não produz novo evento;
  recusas de cancelamento e pedidos não admitidos também não produzem.
- Eventos não incluem números discados, identidades, credenciais,
  chaves de idempotência ou identificadores internos de chamada SIP.
- Autenticação e restrição a `127.0.0.1` continuam exigidas; sem CORS.
- Ao parar o servidor, as transições são limpas; em um novo processo,
  a contagem de sequência também reinicia. Este não é um log de auditoria.
- Eventos de estado SIP da rota `/v1/events` continuam sem vínculo
  definitivo com cada pedido CTI; não inferir que uma transição para
  `in_call` corresponde a um request_id específico.
- Esta versão não contém webhooks nem stream permanente.
- Homologação com CRM, central Dígitro, headset e mídia real pendente.

Testes automatizados cobrem correlação, ordenação, truncamento do
histórico, autenticação HTTP, ausência de eventos duplicados e privacidade.
