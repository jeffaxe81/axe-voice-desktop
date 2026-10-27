# Candidate0.62 — política da sessão e novas regressões de segurança

F2 e COPIAR STATUS agora mostram a política SIP configurada no motor, o processo e a evidência atual de registro. O relatório contém somente quatro booleanos e texto fixo: não inclui conta, destino, senha, caminho da CA ou dispositivo. Alterar o checkbox TLS não muda os dados de uma configuração já aceita. Parar limpa a política; falhar preserva a política selecionada para diagnóstico, sem afirmar sessão/registro atual.

TLS obrigatório é uma política, não uma afirmação de negociação/cifra observada. Registro exige prova nativa atual e estado Registered/Calling/Ringing/InCall; sessão antiga, parada ou falha não fornece prova. O texto sempre informa que o candidato não oferece SRTP e ainda não foi homologado. Exportação permanece local e por ação do usuário, com CRLF, e fica no clipboard até substituição.

## Quatro frentes simultâneas

| Frente | Evidência e mudança |
| --- | --- |
| Destinos/hosts | Host vazio antes de porta, labels com hífen nas bordas e labels vazios eram aceitos. Parser, feedback, conta, chamada e REFER agora usam a mesma sintaxe ASCII de host: até253 caracteres, até63 por label, bordas alfanuméricas. Sem consulta DNS; não comprova servidor existente. |
| Credenciais/ciclo | Falha em libre_init ou mqueue_alloc antes dos hooks retinha senha pendente e deixava Registering. O chamador agora junta a thread, apaga a cópia pendente e publica Failed. Teste com SDK real injeta as falhas apenas no objeto reactor do teste; quatro modos também verificam parar e substituir credencial. Sem criar conta ou enviar SIP. |
| Distribuição | Uma pasta bin redirecionada era omitida pela enumeração recursiva, mas seguida ao preparar o instalador. O gate agora rejeita reparse points no SDK e em seus ancestrais internos ao projeto antes de conferir/copiar arquivos. Regressão usa symlink Linux/junction Windows e DLL sintética nunca executada. Requer pasta de build modificada/redirecionada; não demonstra ataque remoto. |
| Diagnóstico | Matriz de16 combinações, provas de sessão, metadados reais do motor e propagação ao clipboard cobrem política ausente/TLS/UDP e evidências antigas. Verificações permanecem ativas em Release. |

A correção complementa [Segurança0.61](SECURITY-HARDENING.md). Não promete apagar todas as cópias de senha no sistema operacional/terceiros, impedir alterações concorrentes por um atacante local com permissão de escrita, nem substituir pentest ou assinatura de código. Dígitro/headset e demais aceites físicos continuam no [checklist](../RELEASE-CHECKLIST.md). Os gates Windows/SDK/instalador devem corresponder ao commit final do PR.
