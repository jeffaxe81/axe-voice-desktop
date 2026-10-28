# Segurança

## Regras
- não versionar senhas, tokens ou credenciais SIP reais;
- não versionar chaves privadas ou certificados privados;
- evitar dados sensíveis em logs;
- revisar dependências antes de distribuição;
- prever assinatura de código e integridade do instalador;
- manter TLS/SRTP como requisitos de avaliação;
- registrar vulnerabilidades e correções de forma rastreável.

## Repositório público
Como este repositório é público, qualquer dado enviado deve ser tratado como potencialmente visível a terceiros.

## Candidate0.61

A rodada com quatro frentes de testes adiciona recusa de downgrade de destino SIP/SIPS, validação nativa limitada e hashes de arquivos literais no pacote. Resultados, limites e acompanhamento de memória estão em [Segurança0.61](../integration/SECURITY-HARDENING.md). Não equivale a pentest completo, assinatura ou SRTP.

## Candidate0.62

Resumo booleano da política no F2/clipboard, limpeza na falha de partida do reactor, sintaxe de host compartilhada e bloqueio de SDK redirecionado. [Registro e limites](../integration/SECURITY-STATUS.md).

## Candidate0.63

Resultados estruturados de destino com feedback fixo, falha de criação de thread recuperável e schema escalar do manifesto. [Evidência e limitações](../integration/SECURITY-FEEDBACK.md).

## Candidate0.64

Recuperação da configuração sob falha de alocação, limpeza de diagnóstico anterior, codificação CRT dos argumentos e hashes de todos os arquivos SDK. [Registro e limites](../integration/SECURITY-RECOVERY.md).
