# AXE Voice v0.79 — consulta CTI de eventos por solicitação

Esta entrega complementa o histórico de eventos da v0.77 com uma rota
que retorna somente as transições de um pedido específico, eliminando
a necessidade de filtrar o histórico global no CRM.

## Endpoint autenticado

`GET http://127.0.0.1:27652/v1/requests/123/events?after=0`

Autenticação: `Authorization: Bearer <token>`. O parâmetro `after` é
opcional; quando presente, deve ser um inteiro decimal de 64 bits sem sinal.

```json
{
  "latest_sequence": 7,
  "oldest_sequence": 1,
  "gap": false,
  "events": [
    {"sequence": 1, "request_id": 123, "status": "queued"},
    {"sequence": 2, "request_id": 123, "status": "dispatching"},
    {"sequence": 3, "request_id": 123, "status": "submitted"}
  ]
}
```

As sequências são **globais para o journal**, não reiniciadas a cada
`request_id`. Assim, `latest_sequence` pode avançar por eventos de outros
pedidos, mesmo que `events` venha vazio.

`gap=true` tem semântica conservadora: indica que o cursor consultado
ficou anterior ao histórico global disponível. Não confirma que foram
perdidos eventos **deste ID**, apenas que isso é possível.

A solicitação precisa continuar no histórico de até **32 resultados**.
IDs desconhecidos ou já removidos desse histórico retornam 404,
mesmo que uma transição antiga ainda esteja no buffer de 64 eventos.

## Exemplo PowerShell

```powershell
$headers = @{ Authorization = "Bearer $env:AXV_LOCAL_API_TOKEN" }
$id = 123
$cursor = 0
$page = Invoke-RestMethod -Uri "http://127.0.0.1:27652/v1/requests/$id/events?after=$cursor" -Headers $headers
$page.events
$cursor = $page.latest_sequence
```

## Estados e segurança

Os estados são os mesmos da v0.77: `queued`, `dispatching`,
`submitted`, `rejected` e `cancelled`.

- `submitted` significa que o motor SIP aceitou o pedido; **não
  comprova que a chamada foi conectada ou atendida**.
- A consulta ocorre sob o mesmo mutex do journal e jamais retorna
  eventos de outro `request_id`.
- Não são publicados números discados, usuário SIP, segredos, chaves de
  idempotência ou identificadores internos de chamadas.
- A API CTI continua opt-in, restrita a loopback IPv4, protegida por
  token e sem CORS.
- IDs inválidos e cursores inválidos retornam 400, token ausente retorna
  401, origem de navegador retorna 403, e ID desconhecido retorna 404.
- Nada é persistido em disco: reiniciar o AXE Voice apaga os históricos,
  e as sequências/IDs podem reiniciar no novo processo.
- Este endpoint não substitui bilhetagem nem a confirmação de chamadas
  SIP ou de mídia RTP/SRTP.
- Homologação com CRM externo e PABX Dígitro ainda é necessária.
