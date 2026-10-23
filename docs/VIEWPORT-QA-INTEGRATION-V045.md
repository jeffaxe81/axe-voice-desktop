# AXE Voice v0.45 — Integração da rolagem com qualidade em três níveis

Esta entrega parte da branch de integração da PR #12 e incorpora
a rolagem validada na PR #32 **sem descartar** a suíte posterior de
testes `unit`, `blackbox` e `integration`.

## Funcionalidade
- Ajuste de altura da janela ao espaço de trabalho do monitor.
- Rolagem vertical por barra, mouse e foco de teclado.
- Preservação do modo MINI/MAX, escalas DPI e alto contraste.
- Deslocamentos recalculados a partir de posições lógicas originais.
- Smoke Win32 com janela reduzida e política de chamadas bloqueadas.

## Garantias de teste
O arquivo `ui_viewport_policy` agora participa do gate **unit**.
Os testes caixa-preta `--ui-blackbox` permanecem intactos, incluindo
100 destinos sintéticos. Os testes de integração continuam exercitando
a política fail-closed SIP, com 100 entradas sintéticas adicionais.

A taxa de aprovação de 100% dos testes executados não representa
cobertura de código nem homologação com Baresip ou PBX real.

## Verificações ainda necessárias
- CI Windows/MSVC/CMake/CTest e publicação do executável.
- Teste em Windows 10/11, 1366x768, escalas DPI 100–200%, monitor
  secundário, Tab/Shift+Tab, leitor de tela e headset.
- Registro SIP, originação/recepção, RTP/SRTP, áudio duplex e licenças
  do Baresip nativo.

**Laboratório:** não liberar a `main` ou produção.
