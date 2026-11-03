# AXE Voice v0.76 — repetição segura de pedidos CTI

A API local de click-to-call permite que um CRM repita o mesmo pedido
HTTP sem disparar duas chamadas por timeout, falha de rede ou retry.

## Uso

Enviar um identificador opaco por tentativa de discagem no cabeçalho
`Idempotency-Key`. São permitidos 16 a 64 caracteres ASCII, compostos
por letras, números, hífen e sublinhado. **Uma chave deve representar
uma única intenção de chamada.** Não colocar números ou nomes na chave.

```http
POST /v1/calls HTTP/1.1
Host: 127.0.0.1:27652
Authorization: Bearer <token>
Content-Type: application/json
Idempotency-Key: crm_20261102_000001

{"destination":"1001"}
```

Primeira resposta HTTP 202:

```json
{"request_id":4,"status":"queued"}
```

Repetição da mesma chave, mesmo após o motor SIP mudar de estado,
responde HTTP 200 com o mesmo ID e o estado mais recente:

```json
{"request_id":4,"status":"submitted","replayed":true}
```

A API nunca enfileira uma segunda chamada para uma chave que ainda
conste no histórico local. O pedido original pode estar `queued`,
`dispatching`, `submitted`, `rejected` ou `cancelled`.

## Exemplo PowerShell

```powershell
$key = [guid]::NewGuid().ToString('N')
$headers = @{
  Authorization = "Bearer $env:AXV_LOCAL_API_TOKEN"
  "Idempotency-Key" = $key
}
Invoke-RestMethod -Method Post -Uri "http://127.0.0.1:27652/v1/calls" -Headers $headers -ContentType "application/json" -Body '{"destination":"1001"}'
# Se der timeout, repetir EXATAMENTE com a mesma chave.
```

## Condições de segurança

- Sem o cabeçalho, o comportamento existente permanece: cada pedido
  independente pode originar outra chamada quando o ramal estiver apto.
- Uma chave repetida com destino diferente **não** produz outra chamada;
  recebe o resultado do pedido inicial. O CRM precisa usar uma chave
  nova para uma intenção de discagem diferente.
- A retenção é de **até 32 resultados** na memória do processo.
  Após sair do buffer ou reiniciar o aplicativo, a mesma chave pode
  ser tratada como nova. Portanto, não há garantia de exatamente uma
  chamada entre reinícios; é uma mitigação local e de curto prazo.
- A verificação de duplicidade ocorre antes do bloqueio por estado
  SIP ou fila ocupada para evitar retry que se transforme em nova chamada.
- Tokens SIP, números e identificadores internos não aparecem na resposta.
  A chave não é registrada em log e só é mantida no histórico transitório.
- Requisição sem token segue negada; CORS continua bloqueado e a
  porta continua restrita a 127.0.0.1.
- HTTP `submitted` significa pedido aceito pelo motor, não chamada
  atendida nem mídia RTP/SRTP estabelecida.
- Homologação com PABX Dígitro e headset físico permanece pendente.

Testes automáticos cobrem chave inválida, replay nas transições da fila,
estado terminal, ramal indisponível e ausência de segunda discagem.
