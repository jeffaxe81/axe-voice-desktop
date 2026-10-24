# Antecipação 5 meses — Mês 4 — Matriz de segurança do toque v0.60

O CTest `integration_ring_fencing_matrix` percorre **20.000 transições determinísticas** de estado do processo, prova SIP, sessão, token de chamada, tipo da chamada, disponibilidade da saída PC/headset e falha de confirmação de Stop. Um sink sintético proíbe Start enquanto outra saída está ativa e verifica Stop ACK antes de qualquer troca. Mais 1.000 casos de `SipEvidence::None` garantem bloqueio mesmo quando a chamada parece estar tocando.

Sem áudio físico, sem acesso a senhas ou endpoints reais, e sem inferir prova SIP de logs. A CI aprovada representa apenas validação simulada. O motor real Baresip e a homologação Windows/headset/PABX continuam pendentes.
