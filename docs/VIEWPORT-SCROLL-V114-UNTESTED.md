# AXE Voice LAB v0.114 — rolagem adaptada a telas pequenas

Data: 24/10/2026.

## Implementação de desenvolvimento
- Janela Win32 com rolagem vertical somente quando necessário; altura limitada à área de trabalho do monitor.
- Barra de rolagem, roda do mouse, foco Tab em campos abaixo da área visível; políticas de deslocamento em `src/ui_viewport_policy.h`.
- Mudança de aba, modo MINI/MAX e DPI recalculam os limites; coordenadas dos cartões e controles respeitam a rolagem.
- Preservadas a separação de saída PC/headset, os comandos locais da v0.108 e as barreiras de SIP/RTP e autenticação experimental da v0.113.
- CMake e título do frontend atualizados para 0.114.0, sem declarar binário ou release.

## Sem testes (por instrução)
- **Não executados**: build MSVC, CTest, smoke, caixa-preta, integração, monitor físico, headsets, PABX, TLS/SRTP.
- **Não adicionados**: casos automatizados novos nesta rodada.
- Verificar futuramente interações com combobox aberto, DPI 100–200%, monitor pequeno, alinhamento de campos, Tab/Shift+Tab e eventuais expectativas dos testes existentes.
- Não há comprovação de conformidade, compilação nem funcionamento visual real.

A PR histórica #32 contém a inspiração original. Esta implementação é adaptada à linha v0.113 e deve ser incorporada somente na branch de desenvolvimento; jamais considerar automaticamente liberada para produção.
