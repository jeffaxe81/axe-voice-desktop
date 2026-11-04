# AXE Voice v0.81 — descoberta segura da API CTI

A versão 0.81 apresenta um endpoint de **descoberta de funcionalidades**,
para que CRM, aplicações de despacho e outros integradores saibam
exatamente o que o AXE Voice oferece sem inferir capacidades que não
foram implementadas.

## Endpoint

`GET http://127.0.0.1:27652/v1/capabilities`

É obrigatório enviar o cabeçalho `Authorization: Bearer <token>`.
A API permanece **desativada por padrão** e só é iniciada no candidato
Windows nativo quando configurada com o token `AXV_LOCAL_API_TOKEN`.

Exemplo resumido:

```json
{
  "api_version": "1",
  "transport": "http_loopback_ipv4",
  "requires_bearer_token": true,
  "cors": false,
  "features": {
    "click_to_call": true,
    "request_tracking": true,
    "request_events": true,
    "scoped_request_events": true,
    "cancel_queued_request": true,
    "idempotency_key": true,
    "idempotency_key_destination_bound": true,
    "readiness_loss_rejects_queued": true,
    "sip_request_correlation": false,
    "call_completion_confirmation": false,
    "remote_webhooks": false,
    "sip_call_control_api": false
  },
  "limits": {
    "inflight_requests": 1,
    "request_history": 32,
    "request_event_history": 64,
    "sip_event_history": 32,
    "idempotency_key_min_length": 16,
    "idempotency_key_max_length": 64
  },
  "semantics": {
    "submitted": "engine_accepted_not_connected",
    "request_ids": "ephemeral_process_local",
    "event_cursors": "ephemeral_process_local"
  }
}
```

## Consumindo no PowerShell

```powershell
$headers = @{ Authorization = "Bearer $env:AXV_LOCAL_API_TOKEN" }
$cap = Invoke-RestMethod "http://127.0.0.1:27652/v1/capabilities" -Headers $headers
if (!$cap.features.click_to_call) { throw "Click-to-call não suportado" }
if (!$cap.features.sip_request_correlation) {
    Write-Warning "Não há correlação nativa comprovada entre request_id e SIP call_id."
}
```

O endpoint não contém nem revela a senha, token, usuário SIP, números de
telefone, identidades de agentes ou IDs internos das chamadas.

## Limitações fundamentais

- `POST /v1/calls` retornar `submitted` significa somente que o
  **motor aceitou a solicitação**, não que o INVITE foi aceito por um
  destino ou que a chamada foi atendida.
- `GET /v1/calls/current` observa o estado SIP atual **sem provar**
  associação com um determinado `request_id`.
- Os IDs e cursores são válidos somente no processo corrente.
  Uma reinicialização apaga o histórico.
- Não há API remota, CORS, webhooks ou endpoints para atender,
  desligar ou transferir chamadas pela API CTI local.
- A partir da v0.82, repetir a mesma chave `Idempotency-Key` com
  **outro destino normalizado** retorna HTTP 409
  `idempotency_key_conflict`, sem criar uma nova solicitação.
  Para repetir após timeout, manter chave **e** destino idênticos.
- Na candidata v0.83, perda de prontidão SIP rejeita pedidos ainda na fila. O estado `rejected` permanece consultável no histórico limitado; repetir a mesma chave e destino não disca automaticamente após reconexão.
- O contrato de descoberta representa funcionalidades disponíveis
  no software, não a prontidão SIP em tempo real. Use
  `GET /v1/status` para consultar `ready` e cheque o resultado de cada
  operação.
- A integração ainda requer homologação de chamada real, áudio,
  mídia RTP/SRTP, CRM e PABX no laboratório.

## Comportamento de segurança

Todos os endpoints CTI exigem Bearer token, Host local exato,
rejeitam `Origin` e `Referer` e não fornecem CORS. A rota
de capacidades rejeita corpo e `Content-Length` (inclusive zero),
bem como métodos e parâmetros não documentados.

A versão do contrato HTTP (`api_version`) é independente da versão do
aplicativo (`0.81`); integrações devem verificar as flags antes de
tentar funcionalidades novas.
