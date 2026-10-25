> **ARQUIVO HISTÓRICO — PR #18 em rascunho (24/10/2026).** Este documento foi recuperado da branch `feature/axevoice-v039-audio-recovery` e publicado na `main` apenas para rastreabilidade. O código correspondente **não foi mesclado nem compilado ou testado nesta rodada**. Declarações sobre testes no texto original são registros históricos, não evidência atual. Ver [auditoria de PRs](README.md).

# AXE Voice v0.39 — recuperação de headset (laboratório)

## Objetivo
Evitar que uma falha transitória de consulta ao Windows Core Audio apague
as preferências de microfone/saída definidas pelo usuário durante a sessão.

## Comportamento
- Consulta bem-sucedida: aplica seleção prévia se o endpoint ainda existir;
  caso contrário, escolhe o padrão de comunicação ou o primeiro disponível.
- Falha de consulta: mantém os IDs selecionados **somente em memória**,
  desabilita os combos para não sugerir disponibilidade e mostra uma mensagem
  de falha temporária.
- Próxima consulta bem-sucedida: recupera a seleção anterior se possível;
  se o dispositivo realmente desapareceu, aplica fallback explícito.
- Sem dispositivos ativos: controles desabilitados e seleção vazia.
- Atualização automática continua limitada à aba Audio, com lista suspensa
  fechada, conforme PR anterior.

## Validação
`audio_panel_refresh`: cinco casos determinísticos (seleção existente,
falha, recuperação, desconexão e inventário vazio). Compilação manual do
modelo portátil com `g++ -std=c++17 -Wall -Wextra -Werror -pedantic`
aprovada antes do envio. CI Windows e teste real de desconectar/reconectar
headset devem ser verificados na PR.

## Limites de segurança
Esta alteração **não** configura o Baresip, pede permissão de microfone,
registra contas SIP, ativa áudio/RTP/SRTP ou libera chamadas. O executável
permanece um protótipo de laboratório.
