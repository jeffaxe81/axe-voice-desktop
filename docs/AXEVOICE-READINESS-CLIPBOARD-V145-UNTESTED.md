# AXE Voice LAB v0.145 — cópia de prontidão local, sem identificadores

**Origem:** conciliação da PR histórica #21 sobre o frontend LAB v0.144. Entrega **não compilada nem testada**.

## Implementação

- Adiciona **Mais > Copiar checklist de prontidão sem dados pessoais**.
- Reutiliza os seis indicadores booleanos do relatório F2 (CallReadiness); não transfere destino, URI, servidor, usuário, senha, ID de equipamento ou nome de headset.
- Compartilha rotina Unicode de clipboard com teto de 8.192 caracteres e tratamento explícito de falha de alocação, bloqueio e transferência.
- A função existente **Copiar diagnóstico local** continua independente, com conteúdo reduzido; --ui-smoke nunca altera a área de transferência.
- Não há telemetria, escrita em disco ou rede. O conteúdo do clipboard permanece disponível para outros aplicativos até ser substituído.

## Limitações

Somente código estático inspecionado, sem executar CMake, MSVC, CTest, workflow manual, validação de clipboard Windows 10/11 ou homologação. O relatório não comprova SIP real nem RTP/SRTP. A PR #21 também contém testes históricos não portados, mantidos para homologação futura.
