# AXE Voice 0.72 — Click-to-Call e API CTI local

A integração permite que CRM, ERP ou despachador solicitem uma chamada
ao softphone instalado no Windows. Não há serviço externo ou porta LAN.

## Links do navegador

O instalador nativo registra, por usuário do Windows, o esquema axvoice:

```html
<a href="axvoice://call?destination=1001">Ligar para 1001</a>
<a href="axvoice://call?destination=%2B554899999999">Ligar para celular</a>
<a href="axvoice://call?destination=sip%3A1001%40pbx.example.org">Ligar via SIP</a>
```

O navegador pode perguntar se deve abrir o aplicativo. O AXE Voice
preenche o destino e exige uma confirmação do operador antes de iniciar
a discagem. Caso não esteja registrado, o destino é preenchido e o
operador deve registrar e pressionar CHAMAR. Não se deve colocar senha
ou token no link. Um link externo nunca consegue contornar a prova SIP.

## API local (apenas candidato com SDK)

Esta API fica **desativada por padrão**. Para ativá-la, configure uma
variável de ambiente `AXV_LOCAL_API_TOKEN` contendo 32–128 caracteres
aleatórios ASCII (letras, números, _ ou -) antes de iniciar o AXE Voice.
Não reutilizar credenciais SIP.

```powershell
$bytes = New-Object byte[] 32
[System.Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($bytes)
$env:AXV_LOCAL_API_TOKEN = [Convert]::ToBase64String($bytes).TrimEnd('=').Replace('+','-').Replace('/','_')
& "$env:LOCALAPPDATA\Programs\AXEVoiceNativeCandidate\AXE-Voice.exe"
```

Host: `http://127.0.0.1:27652`; header obrigatório:
`Authorization: Bearer <token>`.

### GET /v1/status

Respostas: `200 {"ready":true}` ou `200 {"ready":false}`,
com autenticação. O resultado não comprova uma chamada estabelecida.

### POST /v1/calls

Corpo JSON estrito: `{"destination":"1001"}`
e `Content-Type: application/json`.

```powershell
$headers = @{ Authorization = "Bearer $env:AXV_LOCAL_API_TOKEN" }
Invoke-RestMethod -Method Post -Uri "http://127.0.0.1:27652/v1/calls" -Headers $headers -ContentType "application/json" -Body '{"destination":"1001"}'
```

`202 {"status":"queued"}` confirma apenas que a solicitação entrou
na fila. A GUI revalida registro, sessão, chamada livre e autorização
antes de executar pelo discador existente. Pode haver falha após o 202.

Códigos: 401 token incorreto; 403 origem de navegador ou HTTP proibido;
409 SIP não pronto; 413 corpo excessivo; 415 tipo incorreto;
422 destino inválido; 429 fila ocupada.

## Segurança

- Bind exclusivo em 127.0.0.1 IPv4, sem acesso de LAN e sem CORS.
- Requisições com Origin/Referer/Transfer-Encoding são recusadas.
- JavaScript de uma página remota do CRM não consegue fazer POST direto;
  usar o link `axvoice://` ou um aplicativo local confiável com o token.
- O token não é registrado e não é persistido pelo aplicativo. O operador
  deve evitar colocá-lo em URLs, logs ou páginas web.
- Somente um pedido CTI fica pendente. Formato de destino, autenticação
  e estado SIP são conferidos em cada requisição e na execução.
- O pacote sem SDK mantém CTI desabilitado. Ainda não há homologação
  com a central Dígitro, headset físico, SIP/RTP real ou SRTP.
- Eventos, webhooks, transferências e gestão de tokens são extensões
  futuras, não incluídas nesta versão.
