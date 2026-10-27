# Candidate0.63 — motivos de recusa e partida segura

Chamadas e transferências agora produzem resultado estruturado, mantendo os wrappers existentes que retornam URI. Cada recusa distingue endereço inválido, transporte incompatível, caracteres não suportados, tamanho final acima de512 bytes ou conta inválida para rotear ramal. Mensagens são fixas e dependem somente do enum; não repetem destino, usuário, senha ou caminho. Sintaxe válida não autoriza chamada: as cercas de sessão/comando continuam independentes.

O motor guarda o motivo sob mutex. Tentativa válida, configuração aceita, falha e parada limpam o motivo antigo. A recusa fica vinculada à sessão/chamada e é descartada quando a identidade muda ou outro comando é admitido, para não ocultar falhas atuais de mudo/espera/DTMF. A interface usa esse motivo no diagnóstico de chamada/transferência, sem substituir o resumo booleano F2/COPIAR STATUS. O estado de registro não é apresentado como explicação para um destino incompatível com TLS. O formato suportado e os limites de transporte não foram relaxados.

## Quatro frentes de testes

| Frente | Evidência |
| --- | --- |
| Destino | Matriz cobre roteamento UDP/TLS, razão específica, Unicode, host inválido, limite final512/513 e compatibilidade dos wrappers. Probe com SDK real verifica propagação ao diagnóstico e limpeza da recusa, sem registrar conta. |
| Partida | Exceção por falha de alocação ao criar std::thread escapava de register_account e retinha senha pendente. O método agora captura a exceção e segue join/limpeza/Failed, com mensagem fixa de recuperação. Quinto modo do teste nativo falha uma única alocação; instrumentação fica somente no teste. |
| Pacote | JSON tls:[false,true] era aceito pela comparação de coleção do PowerShell. O manifesto preserva o contêiner JSON e exige um objeto real com campos escalares do tipo correto antes de comparar valores/hashes; também rejeita uma lista contendo um único manifesto válido. Controles válidos e casos de arrays/strings/números/nulos e hash SDK não string são exercitados no gate real antes de staging. Não foi demonstrada substituição de executável TLS ou quebra de hash. |
| Privacidade | Teste de assinatura/tipos mantém entrada somente booleana;2048 combinações de relatório e192 observações de prova verificam texto fixo, CRLF, ausência de garantia SRTP/homologação e cercas existentes. Nenhum vazamento novo foi reproduzido. |

As regressões usam dados sintéticos e verificações ativas em Release. SDK Windows, perfis nativos e instalador reais continuam gates separados do commit final. Não representa pentest completo, apagamento de todas as cópias de segredo no sistema operacional, assinatura de código ou suporte a SRTP. Dígitro/headset permanecem pendentes conforme [checklist](../RELEASE-CHECKLIST.md).
