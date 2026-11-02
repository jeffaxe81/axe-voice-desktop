# AXE Voice v0.70 — teste independente da saída de conversação

A aba **Áudio** possui dois testes locais: **TESTAR TOQUE** (saída de
chamadas recebidas) e **TESTAR VOZ** (alto-falante/headset escolhido para
conversação). Cada teste envia o mesmo sinal curto e baixo de 250 ms pelo
WASAPI do Windows, de forma independente da rede e de qualquer conta SIP.
**Não grava nem reproduz microfone** e não confirma áudio bidirecional.

O teste da saída de voz usa o dispositivo identificado pela seleção de
conversação. Na versão candidata nativa, a opção **Padrão de comunicação**
resolve o endpoint de comunicação presente no inventário atual; a opção
**Padrão multimídia** continua reservada à saída do toque. Uma seleção
explícita desconectada nunca recebe fallback silencioso.

O aplicativo mantém um único player de pré-visualização: os testes não se
sobrepõem. A prévia de conversação é recusada quando o inventário não está
pronto, o teste anterior está em reprodução, há alerta de toque em curso
ou existe uma chamada com token ativo. O botão fica desabilitado nas
verificações sem dispositivos e durante a atualização do inventário.

A CI Windows testa, sem hardware, o mapeamento entre saída de voz e
toque, o caminho sem fallback e a presença/indisponibilidade do novo botão
no teste de interface. O uso em alto-falante/headset reais, dispositivos
Bluetooth e alterações durante chamadas exige homologação manual.

Esta entrega não altera o roteamento nativo de uma chamada, não instala
Baresip, não valida codecs, SRTP ou negociações de áudio e não representa
homologação com central Dígitro.

## Fechamento seguro do teste de instalador

A validação do candidato também testa a concorrência entre instâncias do
assistente Inno Setup, a aplicação e o desinstalador. O fixture foi reforçado
para postar WM_CLOSE apenas uma vez por janela de assistente identificada,
esperar a caixa **Exit Setup** validada por título, texto e botões e
confirmá-la pelo WM_COMMAND/IDYES. Diálogos diferentes continuam recusados.

Nenhum processo é encerrado à força nem se reduz a exigência de que a
instalação recusada preserve o pacote existente. Em timeout, o teste informa
somente os PIDs conhecidos e classes de janelas visíveis, sem coletar
conteúdo arbitrário nem dados da conta. O gate ainda exige aprovação Windows
em todo o commit final da PR.
