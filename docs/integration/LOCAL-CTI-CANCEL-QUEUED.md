# AXE Voice v0.75 — cancelamento CTI de pedido ainda na fila

A API CTI local passa a permitir cancelamento de uma solicitação de
discagem **antes de a interface começar a executá-la**. A implementação
reutiliza a autenticação por token, a fila exclusiva e a documentação
de acompanhamento da versão 0.74.

## Requisição de cancelamento

```http
DELETE /v1/requests/123 HTTP/1.1
Host: 127.0.0.1:27652
Authorization: Bearer <token>
```

Apenas `127.0.0.1:27652` está habilitado; o token é obrigatório.
Não enviar corpo nem cabeçalho Content-Length nessa requisição.

**Se o pedido ainda estiver enfileirado (200):**

```json
{"request_id":123,"status":"cancelled"}
```

**Se já estiver em execução, submetido, rejeitado ou cancelado (409):**

```json
{"error":"already_dispatching"}
```

Outros erros: 400 ID inválido, 401 autenticação inválida,
403 Origin/Referer ou cabeçalhos proibidos e 404 pedido desconhecido
ou retirado do histórico.

## Consulta posterior

A resposta da consulta existente `GET /v1/requests/123` permanece
disponível e passa a poder indicar `status: cancelled`.
O ID continua sendo efêmero e não representa um identificador SIP.

```powershell
$headers = @{ Authorization = "Bearer $env:AXV_LOCAL_API_TOKEN" }
Invoke-RestMethod -Method Delete -Uri "http://127.0.0.1:27652/v1/requests/123" -Headers $headers
```

## Regra de concorrência e limite

- O cancelamento e a retirada da fila são protegidos pelo mesmo mutex.
  Se o pedido for cancelado primeiro, a GUI nunca o recebe.
- Após a GUI retirar o pedido, o cancelamento deixa de ser permitido;
  a API **não envia BYE, CANCEL SIP nem encerra uma chamada em curso**.
- Não existe garantia de cancelamento depois de `dispatching`: respostas
  `409` são definitivas para aquela tentativa.
- Histórico de 32 resultados, sem números telefônicos nem dados pessoais.
- Mesmo que o sistema de CRM já tenha recebido HTTP 202, o operador pode
  cancelar antes do envio; o CRM deve consultar o novo estado.
- A API mantém bloqueio de CORS, token obrigatório e escuta loopback.
- Não equivale a hangup/transferência; não houve homologação em PABX real.

Os testes Windows cobrem cancelamento antes/depois da retirada, estados,
autenticação, ausência de comando SIP após cancelamento e respostas HTTP.
