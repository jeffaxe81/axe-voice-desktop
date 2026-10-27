# Candidate0.61 — verificação de segurança em quatro frentes

Quatro agentes especialistas trabalharam simultaneamente com entradas sintéticas locais, sem credenciais reais ou tráfego à central. Os testes reforçam fronteiras existentes; não constituem pentest completo nem certificação de ausência de vulnerabilidades.

| Frente | Resultado e proteção |
| --- | --- |
| Credenciais/diagnóstico | A tela aceitava caracteres no usuário que o motor rejeitava. Validação compartilhada agora aceita somente letras ASCII, números, _, ., -, +. Backend já bloqueava configuração indevida: não houve injeção demonstrada.A regressão com SDK real também reproduziu retenção da senha pendente após falha anterior à autenticação; o caminho de falha agora apaga essa cópia antes de publicar o estado falho.128 combinações de diagnóstico verificadas; exportação continua somente booleana. |
| Entrada SIP | Reproduzida perda do esquema explícito em ramal `sips:`: normalização podia resultar em UDP. Esquema agora é preservado e transporte incompatível é recusado. Chamadas nativas recusam escapes percentuais, usuários fora da gramática suportada e URI final acima de512 bytes; essas restrições são endurecimento, sem alegar injeção no parser SDK. |
| Autorização | Matriz de sete ações, identidades antigas, estados parados/falhos, mídia e callbacks revogados passou. Mutação removendo igualdade de sessão faz a regressão falhar. Nenhuma chamada não autorizada foi reproduzida. |
| Pacote | Reproduzido hash incorreto aceito quando um nome com colchetes era interpretado como wildcard. Get-FileHash usa LiteralPath, inclusive nas comparações após instalação. Controle válido e arquivo com colchetes correto passam; hash de outro arquivo é recusado. Precondição: artefato/manifesto inconsistente; execução de código não foi demonstrada. |

Regressões C++ usam verificações ativas em Release. O teste PowerShell executa o trecho real de proveniência com árvore sintética e encerra antes de staging, instalação ou execução de binários. Windows, SDK e instalador reais continuam sendo gates separados do PR final.

## Acompanhamento

- O teste de limpeza da senha usa uma senha sintética de997 bytes e falha induzida de módulo antes de criar conta ou enviar SIP. Observa somente alocações C++ vivas da própria configuração sintética; não atesta remoção de todas as cópias em sistema operacional, arquivos de paginação ou terceiros. Não foi demonstrado vazamento externo.
- Instalador permanece candidato sem assinatura. TLS protege sinalização; o candidato não oferece SRTP.
- Dígitro/headset, efeitos remotos e hotplug físico permanecem pendentes conforme [checklist](../RELEASE-CHECKLIST.md).
