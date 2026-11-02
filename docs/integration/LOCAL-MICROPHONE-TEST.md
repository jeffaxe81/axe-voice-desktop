# AXE Voice 0.71 — teste local e efêmero do microfone

O botão **MICROFONE** da aba **ÁUDIO** inicia, somente após um clique
explícito, uma consulta de captura de aproximadamente 850 ms via WASAPI em
modo compartilhado no microfone selecionado.

- O dispositivo escolhido é resolvido pelo inventário mais recente; o
  **Padrão de comunicação** Windows é aceito somente na build nativa e
  apenas se o inventário identifica inequivocamente seu endpoint atual.
  A seleção explícita desconectada não tem fallback silencioso.
- A captura é feita em uma thread independente, não grava arquivo, não
  transmite voz, não envia métricas nem retém amostras após o pacote
  WASAPI ser liberado. Só é mostrado o estado local: sinal detectado,
  silêncio/baixo nível, dispositivo ausente, erro de acesso ou formato.
- Captura e testes locais de toque e voz são mutuamente excludentes.
  Chamadas em andamento e alertas de entrada impedem iniciar o teste;
  deixar a aba, iniciar discagem ou detectar uma nova chamada solicita
  cancelamento. Ao encerrar, os buffers e a sessão WASAPI são liberados.
- Um resultado de **sinal detectado** não prova qualidade, inteligibilidade,
  ausência de eco, permissão futura, roteamento Baresip ou áudio SIP/RTP.
  Silêncio **não significa** que o dispositivo está defeituoso.
- O Windows pode exigir autorização de microfone nas configurações de
  privacidade. Erros nesse acesso não alteram as permissões do sistema.

## Verificações

O teste `microphone_test_policy` cobre escolha de endpoint sem fallback,
PCM16/PCM32/float32, limites de frames, buffer truncado e NaN. O smoke de
interface exige que **MICROFONE** esteja visível e indisponível no modo
sem inventário, sem capturar áudio durante a CI. A CI Windows/SDK deve
aprovar o SHA final da PR; o teste de headset físico e a homologação na
central Dígitro continuam pendentes.

A compilação padrão permanece sem motor SIP; a build nativa continua
sendo um **candidato não homologado para produção**. Não inferir
compatibilidade de SRTP ou mídia bidirecional deste teste.
