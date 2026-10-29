# Silenciar toque recebido — candidate0.67

Nova função de uso diário: botão TOQUE NORMAL/TOQUE MUDO e Ctrl+Shift+S controlam somente o som de chamadas recebidas. Manter aviso visual, ATENDER/ENCERRAR e estado SIP independentes. O padrão é normal a cada execução; opção em RAM, não persistida nem enviada ao servidor. Pode ser escolhida antes de registrar e permanece durante esta execução, inclusive ao trocar a conta.

Reusar o cancelamento cooperativo do IncomingRingtonePlayer. Acrescentar sound_muted=false como quarto argumento de IncomingRingtonePolicy::request; se ativo, retornar pedido vazio antes de copiar endpoint. Não chamar silence(), pois este reconhece uma chamada específica e impediria retomar o mesmo toque ao desativar o modo. Unmute respeita identidade atual, prévia concorrente e silêncio por chamada já reconhecida.

Abordagens: rejeitar INVITEs seria DND e altera chamadas; preferência de som local entrega controle reversível sem mudanças SIP. Escolha do executor: quarto argumento do gate existente e UI com estado explícito junto à duração, visível também em mini. Dividir a linha DTMF em DTMF OFF/ON e TOQUE NORMAL/MUDO sem aumentar janela. DTMF continua SIP INFO, explicado na ajuda. Não alterar mudo do microfone, voz, teste manual ou exportação de diagnóstico.

Garantia: solicitar cancelamento no player; um driver que não coopera pode demorar. Não afirmar corte instantâneo ou audibilidade real. Aceite físico de Windows/headset, central Dígitro, assinatura, SRTP e atualização segura do instalador seguem pendentes.
