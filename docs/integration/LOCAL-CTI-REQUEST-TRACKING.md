# AXE Voice v0.74 — rastreamento de solicitações CTI

A versão 0.74 permite que um sistema de CRM ou despacho acompanhe se um
pedido de discagem autenticado foi enfileirado, encaminhado ao motor SIP
ou rejeitado na validação.

## Fluxo de originação

**POST** `http://127.0.0.1:27652/v1/calls`

Token obrigatório (`Authorization: Bearer <token>`),
`Content-Type: application/json` e corpo:

```json
{"destination":"1001"}
```

A resposta HTTP **202** passa a incluir um identificador efêmero:

```json
{"request_id":1,"status":"queued"}
```

Esse ID **não é o identificador de chamada SIP** e não prova que o
ramal chamou o destino. O servidor pode responder `409` se o
ramal não estiver apto e `429` se já houver pedido pendente/em execução.

## Consulta de resultado

**GET** `http://127.0.0.1:27652/v1/requests/1`

Resposta autenticada:

```json
{"request_id":1,"status":"submitted"}
```

Estados possíveis:

- `queued`: recebido na fila local.
- `dispatching`: a GUI retirou o pedido da fila e está validando a
  sessão e enviando o comando ao motor SIP.
- `submitted`: o motor SIP retornou sucesso ao **aceitar a solicitação**,
  mas o INVITE, o estabelecimento da chamada, a resposta do destino,
  o RTP/SRTP e a gravação não estão comprovados.
- `rejected`: a nova validação da GUI recusou o pedido, o fechamento
  estava em andamento ou o motor rejeitou a solicitação.

Erros HTTP: 400 ID ausente/inválido, 401 token incorreto,
403 origem de navegador proibida, 404 ID inexistente ou expirado.

## Exemplo de integração PowerShell

```powershell
$headers = @{ Authorization = "Bearer $env:AXV_LOCAL_API_TOKEN" }
$call = Invoke-RestMethod -Method Post -Uri "http://127.0.0.1:27652/v1/calls" -Headers $headers -ContentType "application/json" -Body '{"destination":"1001"}'
$id = $call.request_id
$result = Invoke-RestMethod -Uri "http://127.0.0.1:27652/v1/requests/$id" -Headers $headers
$result
```

O integrador pode consultar até obter `submitted` ou `rejected`;
não assumir conclusão quando receber `queued` ou `dispatching`.
Uma interrupção do processo não gera callback nem resposta final.

## Regras e limites

- Apenas loopback IPv4, token válido e processo Windows candidato nativo.
- Fila de **um pedido**, inclusive durante execução na thread da GUI.
- Retenção de no máximo **32 resultados** em memória; consultas anteriores
  retornam 404 após sua saída do buffer. Um reinício do aplicativo apaga
  o histórico e pode reiniciar os identificadores.
- Consulta não expõe o destino telefônico, usuário SIP, senha, call ID,
  identificadores de sessão ou ponteiros internos.
- A GUI e o botão CHAMAR compartilham a mesma rotina de autorização e
  envio; o resultado só muda para `submitted` após retorno verdadeiro
  de `start_call()`.
- API continua sem CORS, sem acesso de navegador remoto e sem webhooks.
- Nenhuma ligação real pode ser declarada homologada sem testes em PABX
  Dígitro, headset físico e mídias RTP/SRTP.

Testes de fila, histórico, concorrência lógica, HTTP autenticado e
estados são executados pela CI Windows e pelo pipeline de SDK nativo.
