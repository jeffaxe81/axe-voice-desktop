# AXE Voice v0.43 — rolagem em telas pequenas

## Funcionalidade
Quando a altura utilizável da tela é menor que a da interface,
a janela se limita à área de trabalho do monitor. Controles permanecem
na mesma estrutura visual e podem ser alcançados pela barra de rolagem,
rodinha do mouse e navegação de foco via teclado.

- Layout original de 96 DPI permanece intacto; offset da rolagem não é
  persistido e é redefinido ao mudar de monitor.
- As abas Teclado/Ajustes/Áudio preservam os dados dos controles.
- MINI/MAX conserva a política de visibilidade e o tamanho da janela é
  limitado ao espaço vertical disponível.
- A área gráfica de cartões e divisórias acompanha a rolagem.
- `ui_viewport_policy` testa limites, controles visíveis, controles
  ocultos e encolhimento do conteúdo.
- Smoke Windows reduz temporariamente a altura de janela, rola e
  verifica bloqueio de discagem.

## Homologação pendente
Teste Windows 10/11 a 100/125/150/200%, tela 1366x768, troca de
monitor, navegação Tab/Shift+Tab, acessibilidade e wheel em combo aberto.
Larguras insuficientes ainda podem exigir um layout horizontal responsivo.

## Segurança
Esta melhoria não habilita o motor nativo, credenciais, áudio RTP/SRTP
nem chamadas. Manter Draft e não mesclar na main até homologação.
