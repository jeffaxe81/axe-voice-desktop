> **ARQUIVO HISTÓRICO — PR #18 em rascunho (24/10/2026).** Este documento foi recuperado da branch `feature/axevoice-v039-audio-recovery` e publicado na `main` apenas para rastreabilidade. O código correspondente **não foi mesclado nem compilado ou testado nesta rodada**. Declarações sobre testes no texto original são registros históricos, não evidência atual. Ver [auditoria de PRs](README.md).

# AXE Voice v0.40 — diagnóstico copiável (laboratório)

Na aba Áudio, o botão **COPIAR STATUS** e o atalho **Ctrl+Shift+D**
exportam a prontidão técnica de chamadas para a área de transferência
do Windows. O checklist F2 continua disponível.

## Escopo e privacidade
- O texto recebe apenas os seis indicadores booleanos de
  `CallReadiness`: sintaxe do destino, sintaxe da conta, motor nativo,
  processo, registro comprovado e mídia comprovada.
- **Não inclui** números, endereços SIP, servidor, nome de usuário, senha
  ou identificadores de dispositivos.
- A operação é local, sem envio de rede e sem escrita em disco.
- CF_UNICODETEXT usa CRLF, UTF-16 e terminador NUL. Se a cópia falhar,
  o aplicativo informa a falha; a memória é liberada corretamente.
- Após copiar, o conteúdo permanecerá no clipboard até ser substituído
  ou apagado pelo Windows/outro aplicativo.

## Validação
- CTest `diagnostic_export`: bloqueio, sucesso, registro pendente, CRLF
  e verificação de ausência de indicadores brutos de SIP.
- Smoke Win32 verifica botão visível e acionamento; intencionalmente
  **não toca no clipboard** no ambiente de CI.
- Integração manual de copiar/colar, teclado, DPI e leitor de tela
  requer teste em Windows 10/11.

## Restrições
O produto permanece em laboratório e usa `UnavailableSipEngine`.
Não houve implementação de SIP real, RTP/SRTP, chamadas nem áudio duplex.
Não realizar merge na main ou uso em produção sem homologação.
