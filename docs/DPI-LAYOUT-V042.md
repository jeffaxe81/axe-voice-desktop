# AXE Voice v0.42 — escalonamento DPI por monitor

- Ativa consciência DPI por monitor V2 em Windows 10/11 quando suportada;
  fallback para consciência de DPI do sistema.
- Guarda a geometria original em unidades 96 DPI. Redimensiona controles,
  fontes, cartões, botões e teclado de forma proporcional.
- Em `WM_DPICHANGED`, recalcula da geometria lógica, sem erros cumulativos;
  mantém modos MINI/MAX e cores de alto contraste.
- CTest `ui_dpi_layout`: DPI 96/120/144/192, arredondamento e limites.
- Smoke Win32: evento DPI sintético 150%, restauração da escala e confirmação
  de chamadas indisponíveis.

Limite conhecido: ainda não há rolagem/refluxo vertical em telas baixas;
escala alta pode fazer a janela completa exceder a altura disponível.
Homologar visualmente Windows 10/11 com múltiplos monitores, 100/125/150/200%,
leitor de tela, teclado e headset.

Nenhuma integração SIP/RTP/SRTP foi habilitada. Somente laboratório;
manter Draft e não mesclar na main.
