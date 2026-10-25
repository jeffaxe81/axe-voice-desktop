# AXE Voice LAB — redefinir headsets da sessão (não testado)

## Funcionalidade

No menu **MAIS**, a nova opção **Redefinir headsets desta sessão...** pede confirmação antes de esquecer os identificadores de microfone, saída de voz e saída de toque **somente em memória**. O Windows, o cofre de credenciais, as configurações do Baresip, o dialer e o registro de SIP não são alterados.

Após a confirmação, a interface limpa as listas/seleções locais; se a aba Áudio estiver visível, agenda a enumeração Core Audio **assíncrona já existente** para recuperar os padrões de comunicação do Windows. Caso contrário, invalida a consulta anterior e atualiza na próxima entrada na aba.

Se houver uma prévia local de som em execução, o reset é **deferido até o worker sinalizar encerramento e ser joined**. A ação pede o cancelamento cooperativo do preview; o reset tem precedência sobre mudanças de rota anteriormente enfileiradas, evitando reativar um dispositivo obsoleto.

## Limites e validação pendente

- Código adicionado ao branch LAB, não à main.
- **Não compilado ou testado nesta entrega**, nem com USB/Bluetooth, Windows 10/11 ou verificação automatizada.
- Nenhuma chamada SIP ou mídia RTP/SRTP é ativada. O recurso não altera dispositivos padrão no sistema operacional.
- Testar posteriormente a combinação de reset durante prévia, cancelamento, mudança de aba, dispositivo desconectado, enumeração com erro e sessão encerrando, além de acessibilidade/menu.
