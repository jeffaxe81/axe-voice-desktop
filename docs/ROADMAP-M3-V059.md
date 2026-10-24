# Antecipação 5 meses — Mês 3 — Watchdog de campainha v0.59

O worker de campainha futuro deixa de repetir infinitamente um tom quando eventos SIP ficam presos em Ringing ou o processo de UI deixa de entregar um Stop. Por padrão são admitidos no máximo **20 ciclos** de som local (com intervalo silencioso de 1100 ms), e o término sinaliza `operational=false` ao coordenador, que recusa retry da mesma chamada. Limites de configuração e tempos são verificados em testes unitários sintéticos; nenhuma chamada ou áudio real foi habilitado.

**Ainda bloqueado:** SDK Baresip nativo, PBX, rota de voz bidirecional, SRTP e teste físico. PR Draft, não mesclar na main antes da validação operacional.
