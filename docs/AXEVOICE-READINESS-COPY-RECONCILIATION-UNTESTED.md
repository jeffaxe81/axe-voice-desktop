# Reconciliação da PR #21: copiar prontidão local sem dados pessoais

Esta adição aplica a funcionalidade ainda faltante da PR histórica #21 ao frontend LAB mais recente, sem alterar o motor SIP.

Em MAIS, **Copiar checklist de prontidão sem dados pessoais** transfere para o clipboard Unicode somente o relatório de seis estados de prontidão já apresentado pelo botão F2. Dados digitados (ramal, usuário, senha, servidor, endereço e endpoint de headset) não são colocados no texto. A rotina de clipboard limita o relatório a 8.192 caracteres e preserva o tratamento de falhas Win32. No smoke mode não modifica a área de transferência.

A opção anterior de **Copiar diagnóstico local sem dados pessoais** continua funcionando, agora usando a mesma rotina de transferência.

**Estado: não compilado, sem testes ou validação no Windows.** Clipboard pode ser lido por outros programas até que seu conteúdo seja apagado ou substituído. O relatório não prova registro SIP, RTP/SRTP ou chamadas. Os testes históricos da PR #21 não foram portados e continuam pendentes para QA.
