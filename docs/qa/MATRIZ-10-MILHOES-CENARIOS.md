# AXE Voice — bateria determinística de 10 milhões de cenários (2026-11-08)

**Natureza da evidência:** 10.000.000 execuções de *cenários de propriedades sintéticas e funções puras*, não dez milhões de chamadas telefônicas nem dez milhões de testes GUI físicos. A execução deve ser comprovada pelo **Windows CI** (artifato `AXE-Voice-Policy-10M-Results`) no SHA exato da revisão. O log/JSON contêm semente fixa, contagem executada e falhas, sem credenciais SIP.

## Matriz executável

| Domínio | Cenários | Política observada |
|---|---:|---|
| Rolagem e limites visíveis | 2.000.000 | Saturação, seleção da área visível, conteúdo menor/maior que janela, dimensões negativas/extremas |
| DPI e rodapé | 2.000.000 | Escala 96–384 DPI, saturação de INT_MIN/INT_MAX, contenção do texto no painel |
| Teclado de discagem | 2.000.000 | Teclas permitidas, seleção, backspace, limpar, limite de tamanho, rejeição de caracteres SIP especiais |
| Estados SIP e callbacks | 2.000.000 | Prova de sessão, status efetivo, permissão de chamadas, rejeição de eventos obsoletos e não verificados |
| Regras de entrega e codecs | 2.000.000 | 8 gates de engenharia × 6 externos, produção bloqueada sem evidência, G.729 condicionado à validação |
| **Total** | **10.000.000** | **Cinco domínios de lógica pura** |

Os testes não têm servidor SIP, usuário, senha, interface real, mídia física nem ligação externa; não podem comprovar interoperabilidade, UX real ou cobertura total de código. Os demais testes existentes do CTest continuam ativos para módulos adicionais (áudio, contatos, instalador, CTI, segurança e adaptação Baresip), com 1.000 registros SIP de laboratório no pipeline SDK separado.

## Correções decorrentes da revisão

- `src/ui_viewport_policy.h`: todo cálculo de scroll usa intermediários de 64 bits e limita o resultado à área válida antes de convertê-lo para `int`, eliminando transbordamento quando `content_height`, `bottom + margin` ou `current + visible` recebem valores extremos.
- `src/ui_dpi_policy.h`: escala DPI calculada com inteiros de 64 bits, arredondamento simétrico e saturação nos limites de `int`, incluindo o caso `INT_MIN` (que não pode ser negado como `int`).
- Testes direcionados de regressão em `tests/test_ui_viewport_policy.cpp` e `tests/test_ui_dpi_policy.cpp`.

## Como gerar evidência no Windows

```powershell
pwsh ./scripts/build-windows.ps1
./build/Release/axv_10m_policy_stress.exe ./build/axe-voice-policy-10m.json
```

No GitHub Actions, o workflow `AXE Voice Windows CI` executa o mesmo programa **uma vez por SHA**, valida que o JSON contém exatamente `executed_scenarios=10000000`, `failed_scenarios=0`, `live_pbx_tested=false` e `production_approved=false`, e publica o artefato. O certificado `AXE Voice | aceite de engenharia por SHA` passa a rejeitar evidência ausente, pulada ou de commit antigo.

**Não confundir 10 milhões de cenários simulados com homologação real de todas as funcionalidades.** O formulário offline Windows continua sendo necessário para execução física de instalação, acessibilidade, headset, temas e design aprovado. Registro/chamadas/RTP com PBX real permanecem fora do escopo de engenharia e pendentes de produção.
