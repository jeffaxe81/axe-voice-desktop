# Antecipação 5 meses — Mês 2 — Cancelamento cooperativo WASAPI v0.58

O worker de campainha passa a encaminhar **o mesmo sinal de cancelamento atômico** ao callback de renderização e ao player local WASAPI, cuja implementação já verifica cancelamento antes de acessar o endpoint e durante a reprodução. O Stop ACK continua obrigatório antes de selecionar outro dispositivo. O teste `cooperative_ringtone_cancel` confirma parada cooperativa e reinício sequencial em endpoints sintéticos, sem som real.

**Laboratório apenas:** o renderer de campainha não foi habilitado no frontend; o registro SIP/RTP ainda requer SDK auditado e validação em headset/PABX físicos.
