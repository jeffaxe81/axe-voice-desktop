# Destinos recentes da sessão — candidate0.66

Entregar três funções de uso diário: lista dos últimos10 destinos de discagens aceitas localmente pelo motor, escolha para preencher o campo de destino e limpeza manual. Usar botão RECENTES no cabeçalho de discagem e atalho Ctrl+H, com menu acessível do Windows. Escolher nunca chama automaticamente: CHAMAR continua autorização atual + recusa independente do motor.

Guardar somente texto originalmente digitado, válido, ASCII nativo e até128 caracteres; preservar sip/sips e URI sem esquema como digitadas para não truncar uma normalização maior no campo. Deduplicar por texto exato, ordem mais recente primeiro, renovar token em cada aceitação. RAM apenas; sem persistência, copiar status ou credenciais. Não representar chamadas recebidas, atendidas, duração, cobrança ou confirmação remota.

Contexto válido: motor Running, VerifiedNativeApi, sessão não zero e SIP Registered/Calling/Ringing/InCall. Preservar lista durante chamadas; limpar em sessão diferente, perda de prova, parada ou outros estados. Registrar novo destino exige Registered sem call_id e comando aceito. Seleções antigas usam token+observação atual, nunca índice sozinho. Token não volta a zero ao limpar. Falha de alocação não modifica a lista da mesma sessão nem impede uma chamada já aceita.

Abordagens: histórico persistente exige consentimento/dados adicionais e uma janela nova amplia interface; menu nativo em memória entrega uso imediato e acessibilidade padrão sem mudar geometria geral. Escolha do executor: menu em RECENTES, mensagens fixas e quatro frentes independentes de testes. Atualização do instalador entre versões continua em acompanhamento separado.

Privacidade: limpar remove registros lógicos e esvazia buffers ativos da lista; não comprova apagamento forense de cópias do SO ou capacidade não ativa. Homologação Dígitro/headset, assinatura e SRTP permanecem pendentes.
