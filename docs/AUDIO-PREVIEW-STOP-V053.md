# AXE Voice v0.53 — interrupção dos testes locais de áudio

## Interface Windows
- Durante um teste de voz, o botão muda de **TESTAR VOZ** para
  **PARAR VOZ**. Durante o de toque, muda de **TESTAR TOQUE** para
  **PARAR TOQUE**.
- Clicar novamente solicita cancelamento sem aguardar no thread da interface;
  o segundo botão fica indisponível e não pode parar um worker de outro papel.
- Durante a parada, o botão exibe **PARANDO...** e só volta a habilitar após
  o worker terminar e os resultados serem recolhidos pelo timer.
- Trocar dispositivo de saída, inverter os destinos, atualizar inventário
  ou sair da aba Áudio solicita cancelamento do teste em andamento.
- Encerrar a aplicação cancela primeiro e **depois** aguarda o worker.

## Backend de laboratório
`play_selected_ringtone_preview` recebe sinal opcional de cancelamento
atômico. Se o sinal já estiver acionado, retorna **Cancelled** antes de
inicializar COM ou abrir dispositivo. Durante o teste, verifica o sinal
em intervalos curtos (até 10ms entre verificações na fase de reprodução)
e interrompe o WASAPI. Resultados de cancelamento são informados
separadamente de falha de driver ou sucesso.

## Segurança e testes
- O modo `--ui-smoke` / `--ui-blackbox` continua proibindo reprodução.
- Os IDs de dispositivos são locais e não são persistidos ou enviados a SIP.
- O teste `audio_preview_cancel_policy` valida 32 combinações de início,
  cancelamento prévio, modo smoke e função voz/toque.
- `ringtone_preview` testa cancelamento antecipado usando identificador
  fictício, sem tocar nem acessar hardware.

**Não implementa** chamada SIP, reprodução de voz telefônica, ringtone
de ligação recebida, nem RTP/SRTP. A validação auditiva com headset e
caixas reais Windows 10/11 ainda é indispensável.
