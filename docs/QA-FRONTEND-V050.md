# AXE Voice — QA de frontend v0.50

## Objetivo
Confirmar mensagens exibidas por um processo Windows independente, não apenas o estado habilitado de botões.

## Interfaces de teste
IDs estáveis Win32: **2501** para o feedback de destino/conta e **2502** para o banner do motor/registro SIP.

## Critérios automatizados
- Teste unitário: 1000 pares de destinos válidos/inválidos; mensagens esperadas e validação sintática SIP.
- Caixa-preta: processo real Windows distinto; 100 entradas de destino; mensagens de erro e sucesso de sintaxe; verificação de bloqueio de discagem e AXV-SIP-003; banner SIP indisponível; validação de conta e navegação.
- Integração: gates anteriores de isolamento de sessões, controle de chamadas e eventos.
- CI: exigidos 100% de testes aprovados por categoria; não liberar o artefato caso haja falhas.

**Limites:** sem registro SIP real, autenticação com PBX, áudio bidirecional, RTP/SRTP, teste físico de headset ou cobertura de código de 100%.
