# Exemplo de integração — AXE Voice CTI no PowerShell

Este módulo é um **cliente local de demonstração**, não um servidor nem
uma aplicação de produção. Ele utiliza apenas a API que está escutando em
`127.0.0.1:27652`, mediante token definido no AXE Voice e no ambiente
do PowerShell. Nunca publique esse token em JavaScript de navegador
ou em um servidor remoto.

## Iniciar

```powershell
# Configurar AXV_LOCAL_API_TOKEN com um token de 32 a 128 caracteres
# permitido pela configuração nativa, no ambiente apropriado.
Import-Module .\examples\cti\AXEVoiceCti.psm1 -Force
$cap = Get-AXVCtiCapabilities
$cap.features
```

## Efetuar pedido de Click-to-Call

```powershell
$key = [guid]::NewGuid().ToString('N')
$req = Start-AXVCtiCall -Destination '1001' -IdempotencyKey $key
$req
# Em caso de timeout de rede, repita com o MESMO $key:
# $req = Start-AXVCtiCall -Destination '1001' -IdempotencyKey $key
$status = Get-AXVCtiRequest -RequestId $req.request_id
$events = Get-AXVCtiRequestEvents -RequestId $req.request_id -After 0
$events.events
```

Para cancelar **somente se ainda estiver na fila**:
`Stop-AXVCtiQueuedRequest -RequestId $req.request_id`.

O módulo não oculta erros HTTP; código 409 indica que uma ação
não é mais permitida no estado corrente. O estado `submitted`
não significa que a chamada foi conectada ou atendida.

## Limites

- O token precisa estar provisionado localmente no candidato nativo.
- O código não executa polling automático ou trabalho em segundo plano.
- Em reinicializações do AXE Voice, os IDs e histórico em memória
  desaparecem, assim como a proteção limitada de idempotência.
- O módulo de demonstração utiliza IDs positivos de até 64 bits
  com sinal; o servidor aceita inteiros de 64 bits sem sinal.
- O módulo usa JSON com escape seguro para o destino informado.
- Não implementa webhooks, transferência, atendimento ou desligamento
  de chamadas. Homologação SIP, áudio e CRM real ainda são necessárias.
