# Candidate0.56 — diagnóstico local copiável

Na aba Áudio, **COPIAR STATUS** ou **Ctrl+Shift+D** copia o relatório de prontidão. O atalho funciona também nas outras abas. F2 continua exibindo o relatório. A disponibilidade de Chamar usa a mesma regra da interface; não certifica mídia nem aceite da central.

A entrada do exportador contém somente sete indicadores booleanos: sintaxe de destino/conta, motor disponível, processo em execução, registro comprovado, mídia verificada e botão Chamar disponível. Não recebe valores de conta, destino, senha, endereço, dispositivo ou log bruto. O texto usa CF_UNICODETEXT com CRLF e terminador NUL. A cópia ocorre somente após ação do usuário, sem arquivo ou envio de rede. O conteúdo permanece até ser substituído; histórico/sincronização seguem as configurações do Windows.

O serviço prepara a memória antes de abrir a área de transferência. Falhas de alocação, bloqueio, abertura, limpeza e transferência devolvem erro e liberam os recursos ainda pertencentes ao aplicativo. Se a limpeza suceder e a transferência falhar, o conteúdo anterior pode já ter sido removido. A interface informa a falha para nova tentativa.

Testes do formato verificam os indicadores e as quebras de linha; stubs WinAPI isolados verificam propriedade de memória e todos os caminhos de falha. Smoke padrão/nativo exercita botão e comando de atalho sem alterar a área de transferência real. A cópia/cola, combinação física de teclas e acessibilidade em Windows físico continuam pendentes, junto com a homologação na Dígitro e no headset. Nenhuma aprovação de produção é implícita.
