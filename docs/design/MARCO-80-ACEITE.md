# AXE Voice — marco de 80% no design e na ergonomia

Data: 2026-11-07. Referência: edição **Essencial (Opção 01)**, janela Win32 de 355 × 602 pixels lógicos e edição WinUI 3 (prévia).

**Medição:** 8 de 10 critérios internos de implementação e testes aprovados = **80% do estágio de design**.
Cada critério possui o mesmo peso. Isto **não** é percentual de desenvolvimento do aplicativo inteiro, de conclusão do SIP/Baresip ou de homologação de produção. A escolha e pesos dos 10 critérios são uma convenção explícita de gestão, não uma aferição independente de experiência visual.

| Nº | Critério verificável | Resultado | Evidência |
|---:|---|---|---|
| 1 | Dimensões compactas Essencial e ajuste ao espaço útil em DPI 96/120/144/192 | [x] | [PR #225](https://github.com/jeffaxe81/axe-voice-desktop/pull/225); `essential_compact_smoke` |
| 2 | Sem rolagem vertical, controles fora do cliente ou colisão nos estados testados | [x] | [PR #225](https://github.com/jeffaxe81/axe-voice-desktop/pull/225), 138/138 Windows CI |
| 3 | Rótulos de botões medidos pela fonte real, sem truncamento nos estados testados | [x] | `essential_labels_fit` na PR #225 |
| 4 | Controles de origem, recepção e encerramento posicionados corretamente conforme sessão SIP simulada | [x] | `essential_compact_smoke` na PR #225 |
| 5 | Tema claro/escuro Win32 sem alteração indevida da geometria | [x] | `essential_compact_smoke` na PR #225 |
| 6 | Paletas light/dark completas na prévia WinUI 3, com recursos sem valores ausentes | [x] | [PR #228](https://github.com/jeffaxe81/axe-voice-desktop/pull/228), `check-winui3-theme.ps1`, build WinUI3 |
| 7 | Logo compacto aprovado incorporado em título, cabeçalho e bandeja do Win32 | [x] | [PR #230](https://github.com/jeffaxe81/axe-voice-desktop/pull/230), recurso ICON 101, CI Win32 138/138 |
| 8 | Mesmo logo aprovado incluído no cabeçalho e no pacote da prévia WinUI 3 | [x] | [PR #231](https://github.com/jeffaxe81/axe-voice-desktop/pull/231), `check-winui3-brand.ps1`, compilação/publish WinUI3 |
| 9 | Comparação pixel a pixel de capturas reais das edições contra todas as referências aprovadas | [ ] | Pendente — requer imagens do EXE em execução, não apenas os modelos visuais |
| 10 | Revisão presencial da ergonomia, bandeja, ícone, preferências, headset e instalador em Windows físico | [ ] | Pendente — ambiente e teste final do usuário/equipe |

## Escopo excluído do 80%

- Não comprova que o motor Baresip registra no SIP da Dígitro; testes de loopback e ABI não substituem comunicação externa.
- Não comprova áudio bidirecional real, qualidade RTP, NAT, recepção INVITE, SRTP ou certificado.
- WinUI 3 permanece **preview sem ações SIP habilitadas**, apesar de compartilhar a camada de apresentação.
- Não substitui assinatura de código, testes de instalação e atualização em máquina real nem aprovação formal para produção.
- A CI valida a geometria com controles Win32. **Uma captura real é necessária** para inspecionar alinhamento perceptivo, antialiasing, contraste e fidelidade ao modelo FaleWEB fornecido.

## Próximas ações para avançar além de 80% no design

1. Capturar automaticamente ou manualmente o EXE real em claro/escuro, quatro DPIs e estados de chamada; anexar evidências à issue #227.
2. Comparar esses resultados com as referências aprovadas, com tolerâncias explícitas, corrigindo divergências visuais.
3. Inspecionar o instalador nativo e a edição WinUI3 em Windows 11 real; validar ícone do Menu Iniciar, tray, preferências e acessibilidade.
4. Tratar funcionalidade/homologação SIP com métricas **separadas** da conclusão de design, conforme docs/RELEASE-CHECKLIST.md.

O marco de 80% só é considerado válido enquanto os pipelines dos commits associados permanecem concluídos com sucesso. Não usar esta medição para afirmar que o softphone inteiro está 80% pronto.
