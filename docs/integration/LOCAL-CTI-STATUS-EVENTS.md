# AXE Voice v0.73 — Estado da chamada e eventos locais

Esta entrega amplia a API CTI da v0.72 com consultas autenticadas e
sem informações pessoais. A API continua **opcional**, restrita a
`127.0.0.1:27652`, sem CORS, com token `AXV_LOCAL_API_TOKEN`.

## Endpoints novos

### GET /v1/calls/current

Consulta a condição SIP apresentada pelo motor nativo:

```json
{"phase":"in_call","registered":true,"active_call":true,"sequence":3}
```

Os estados possíveis para `phase` são: `offline`, `unverified`,
`registering`, `registered`, `dialing`, `ringing`, `in_call`
e `failed`. Quando o estado não foi comprovado pela sessão nativa
atual, a resposta é `unverified`, nunca `registered` por inferência.

O atributo `sequence` indica a última transição observada pelo
AXE Voice desde o início do processo, não um identificador SIP.

### GET /v1/events?after=0

Consulta as transições **sem webhook remoto** ou transmissão externa:

```json
{
  "latest_sequence": 3,
  "oldest_sequence": 1,
  "gap": false,
  "events": [
    {"sequence": 1, "phase": "unverified"},
    {"sequence": 2, "phase": "registered"},
    {"sequence": 3, "phase": "in_call"}
  ]
}
```

O parâmetro `after` é opcional e representa o último número de sequência
recebido. A resposta contém eventos com sequência superior ao valor
informado. O histórico é limitado aos **32 últimos eventos** na memória
do processo. `gap=true` significa que eventos anteriores já saíram
do buffer e que o integrador precisa consultar novamente o estado atual.

### Exemplo PowerShell

```powershell
$headers = @{ Authorization = "Bearer $env:AXV_LOCAL_API_TOKEN" }
Invoke-RestMethod -Uri "http://127.0.0.1:27652/v1/calls/current" -Headers $headers
Invoke-RestMethod -Uri "http://127.0.0.1:27652/v1/events?after=0" -Headers $headers
```

## Segurança e precisão

- As respostas não contêm usuário SIP, números de telefone,
  identificação de contato, credenciais nem tokens internos de chamada.
- Só é declarada evidência de registro SIP proveniente do motor nativo.
- As transições são amostras do estado publicado na GUI; mudanças
  muito rápidas entre atualizações podem não aparecer.
- O histórico é **efêmero**, não é uma trilha de auditoria durável nem
  substitui bilhetagem ou relatórios do PABX.
- Sem autenticação, a API responde `401`. Requisições de páginas com
  `Origin` ou `Referer` são rejeitadas como na v0.72.
- O status não confirma qualidade de áudio, entrega de mídia,
  estabelecimento de RTP/SRTP ou homologação em central Dígitro.
- Sem webhooks nesta versão: para receber mudanças, o sistema local deve
  consultar periodicamente a API; autenticação e chamadas continuam
  sujeitas à política estabelecida na v0.72.

## Critérios para integrar

Windows CI, integridade MicroSIP e SDK nativo aprovados no commit exato
da PR. Testes determinísticos verificam a rejeição de evidência SIP não
confirmada, ordem de transições, limite de 32 eventos, lacunas de sequência,
parsing de cursor e acesso HTTP autenticado.

Homologação física em headset, chamadas reais e integração com CRM externo
permanecem pendentes. O recurso é candidato de laboratório, não uma
liberação de produção.
