# Antecipação 5 meses — Mês 1 — Worker cancelável (v0.57)

Integra o `CancellableIncomingRingtoneSink` isolado à linha de qualidade do frontend, apoiado na PR #71. O worker recebe callback injetável, controla thread única, solicita cancelamento, aguarda confirmação de parada e impede duas saídas simultâneas. O coordenador pode suspender nova saída diante de falha. Testes unitários e de integração usam um renderizador simulado e eventos SIP sintéticos.

A UI continua com `DisabledIncomingRingtoneSink` e `SipEvidence::None`; o worker **não é ativado automaticamente** e não há SIP/RTP real. Compilação Windows comprova compatibilidade do código, não áudio real. Não mesclar em `main` antes de homologação.
