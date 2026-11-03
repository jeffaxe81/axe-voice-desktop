# AXE Voice v0.78 — diagnóstico seguro de desinstalação do candidato

Esta versão acrescenta validação de pré-desinstalação no pipeline de
empacotamento nativo Windows. A mudança não altera o softphone ou
o comportamento do Inno Setup em máquinas dos usuários.

## Problema acompanhado

Na PR #165 (v0.77), a primeira tentativa do pipeline nativo falhou
com `Candidate silent uninstall failed` na etapa de aceitação do
instalador. A segunda tentativa, no mesmo commit, passou integralmente.

**A causa ainda não foi comprovada.** Possíveis atrasos na liberação do
marcador da GUI ou no desbloqueio do executável estão sendo testados
como hipóteses, não tratados como explicação definitiva.

## Verificações adicionadas

Antes de invocar a desinstalação silenciosa, o script exige que:
- O marcador Windows da aplicação tenha desaparecido.
- O marcador Windows do instalador tenha desaparecido.
- O executável instalado esteja acessível em modo de compartilhamento
  exclusivo (teste sem escrita).

Os critérios são examinados repetidamente, por no máximo 15 segundos.
**Se algum deles não puder ser comprovado, o pipeline falha sem executar
a desinstalação.** O teste não encerra processos e não altera
permissões, executáveis, mutexes ou o guard de segurança do Inno.

A execução silenciosa registra seu código de saída e utiliza o
argumento `/LOG` do Inno Setup. Se falhar, os últimos 40 registros
do instalador são impressos na CI para diagnóstico; código não zero
continua reprovando o gate. O log fica fora da pasta instalada e não
inclui tokens de autenticação SIP ou CTI.

## Regressões

O teste `tests/test_native_uninstall_readiness.ps1` valida:
- Ausência dos marcadores e executável livre.
- Bloqueio quando marcador de aplicação está presente.
- Bloqueio quando marcador de setup está presente.
- Bloqueio com arquivo aberto por outro processo/handle.
- Recusa caso o executável não exista.
- Limite de espera, sem cancelamento do controle de segurança.
- Retorno ao estado pronto após liberação dos bloqueios.

A issue #166 continua aberta até haver evidência suficiente para
explicar a intermitência e estabilidade demonstrada em novas execuções.
