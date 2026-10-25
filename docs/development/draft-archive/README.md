# AXE Voice — triagem das 13 PRs Draft históricas

**Conferência:** 24/10/2026. **Escopo:** rascunhos em aberto no repositório `jeffaxe81/axe-voice-desktop`. Este arquivo é uma cópia de gestão na `main`, **não** um merge do código dos rascunhos.

## Resultado objetivo

As 13 PRs retornaram `mergeable=false` e `mergeable_state=dirty` pela API GitHub na base original. A maioria mira `feature/baresip-desktop-bootstrap` em vez da `main`. A PR #12 mira `main`, mas também conflita: são 202 commits, 123 arquivos e implementação antiga v0.51, enquanto a branch de desenvolvimento mais recente alcançou v0.144. Esse material não pode ser fundido automaticamente como se fosse uma atualização linear.

A comparação dos heads antigos com a branch atual `feature/axevoice-v073-sip-activation-gate` também retornou histórico `diverged` para todos os 13 rascunhos. Isto **não prova** que todas as mudanças antigas já foram absorvidas ou são desnecessárias: cada função deve ser confrontada com o código atual.

## Reconciliação parcial concluída

A [PR #110](https://github.com/jeffaxe81/axe-voice-desktop/pull/110) foi integrada à branch de desenvolvimento em v0.134 com três portabilidades de código: limite do lote de eventos SIP (#39), preservação de seleções após falha de inventário de áudio (#18) e recuperação da preferência explícita de headset/voz/toque (#16). **Não mesclou integralmente nem fechou as três PRs antigas** e não executou testes. Consulte [a nota de reconciliação](../AXEVOICE-DRAFT-RECONCILIATION-V131-V134-UNTESTED.md).

A [PR #112](https://github.com/jeffaxe81/axe-voice-desktop/pull/112) também foi mesclada **somente no desenvolvimento**, portando a validação de `call_get_ua` da PR #28 e o relatório restrito a indicadores da PR #21. Os rascunhos antigos permanecem abertos. [Nota técnica v0.139](../AXEVOICE-DRAFT-RECONCILIATION-V135-V139-UNTESTED.md). Nenhum teste ou build foi realizado nesta etapa.

A [PR #114](https://github.com/jeffaxe81/axe-voice-desktop/pull/114) foi mesclada na **branch de desenvolvimento** na v0.144, aproveitando mudanças de dispositivo padrão e debounce (#30/#40) e conferência da sessão nativa de callback (#31). Nenhum dos rascunhos originais foi mesclado por inteiro ou encerrado. [Nota técnica v0.144](../AXEVOICE-AUDIO-WATCHER-CALLBACK-FENCE-V140-V144-UNTESTED.md). Sem compilação ou testes nesta rodada.

## Matriz de decisão

| PR antiga | Função principal | Arquivos do diff | Resultado | Próxima ação técnica |
|---|---|---:|---|---|
| [#12](https://github.com/jeffaxe81/axe-voice-desktop/pull/12) | Fundamentos Windows LAB/Baresip v0.51 | 123 | Conflito contra `main` | Reconciliar arquitetura completa, compilar/homologar antes da promoção de código |
| [#14](https://github.com/jeffaxe81/axe-voice-desktop/pull/14) | Identidades de conta e chamada SIP | 10 | Conflito na base histórica | Comparar controle de identidade com reducer e callback atuais |
| [#15](https://github.com/jeffaxe81/axe-voice-desktop/pull/15) | Ingresso nativo de eventos | 9 | Conflito na base histórica | Evitar duplicar ingresso de callbacks; examinar equivalência |
| [#16](https://github.com/jeffaxe81/axe-voice-desktop/pull/16) | Recuperação automática de headset | 10 | Portabilidade parcial via #110; PR original conflita | Restante da integração e testes pendentes |
| [#18](https://github.com/jeffaxe81/axe-voice-desktop/pull/18) | Recuperação headset + diagnóstico | 10 | Portabilidade parcial via #110; PR original conflita | Validar fluxo da interface e restante do diagnóstico |
| [#21](https://github.com/jeffaxe81/axe-voice-desktop/pull/21) | Exportação segura de prontidão | 7 | Formatador portado via #112; PR antiga conflita | Restante da UI/documentação/testes pendente |
| [#23](https://github.com/jeffaxe81/axe-voice-desktop/pull/23) | Pipeline e validação de proveniência | 15 | Conflito na base histórica | Inspecionar integração com ciclo de vida atual |
| [#28](https://github.com/jeffaxe81/axe-voice-desktop/pull/28) | Verificação da UA proprietária no SDK | 4 | Proteção call_get_ua portado via #112; PR antiga conflita | SDK/ABI Windows e restante da diff pendentes |
| [#30](https://github.com/jeffaxe81/axe-voice-desktop/pull/30) | Atualização assíncrona de dispositivos | 18 | DBT_CONFIGCHANGED portado via #114; PR antiga conflita | Fluxo assíncrono e restante das mudanças pendentes |
| [#31](https://github.com/jeffaxe81/axe-voice-desktop/pull/31) | Sessões de callback Baresip e auditoria SDK | 36 | Validação adicional da sessão portado via #114; PR antiga conflita | ABI/SDK e demais mudanças de callback pendentes |
| [#36](https://github.com/jeffaxe81/axe-voice-desktop/pull/36) | Rolagem da interface e gates de qualidade | 7 | Conflito na base histórica | Isolar eventual melhoria não presente, sem reverter tela nova |
| [#39](https://github.com/jeffaxe81/axe-voice-desktop/pull/39) | Matriz fail-closed e cenários sintéticos | 6 | Limite de eventos portado via #110; PR original conflita | Restante da suíte e documentação/testes pendentes |
| [#40](https://github.com/jeffaxe81/axe-voice-desktop/pull/40) | Headset hotplug e Core Audio | 29 | Observador de endpoints portado via #114; PR antiga conflita | Validar troca real de dispositivos e restante da UI |

**Status da solicitação "merge em main sem testes":** é possível publicar apenas documentação e rastreabilidade sem trazer uma pilha de código conflitante; isso foi feito nesta ramificação de governança. **Nenhuma das 13 PRs Draft de código foi mesclada integralmente**, nem marcada como pronta, nem encerrada por esta triagem. Permanecem abertas para reconciliação individual.

## Arquivos históricos preservados na main

Os arquivos desta pasta mantêm referências antigas vindas diretamente das respectivas branches de rascunho. Sua presença na `main` **não significa entrega, execução de teste ou disponibilidade da funcionalidade**:

- [#39 — proteção fail-closed](SIP-FAIL-CLOSED-REGRESSIONS.md)
- [#36 — viewport e qualidade](VIEWPORT-QA-INTEGRATION-V045.md)
- [#18 — recuperação de headset](AUDIO-RECOVERY-V039.md) e [diagnóstico](DIAGNOSTIC-EXPORT-V040.md)
- [#15 — eventos SIP nativos](NATIVE-PIPELINE-LAB.md)
- [#23 — endurecimento do pipeline](PIPELINE-HARDENING-V039.md)
- [#31 — integridade Baresip](BARESIP-SDK-INTEGRITY-GATE.md)
- [#28 — callbacks SDK](PR-28-BARESIP-SDK-EVENT-ADAPTER.md)
- [#14 — arquitetura SIP](PR-14-BARESIP-INTEGRATION.md)
- [#12 — checklist legado](PR-12-RELEASE-CHECKLIST.md)
- [#30 — changelog de áudio](PR-30-CHANGELOG.md)

As PRs #16, #21 e #40 repetem versões antigas do documento geral do frontend e ficam referenciadas na matriz, evitando adicionar três manuais potencialmente contraditórios à `main`.

## Para concluir os merges de código

A próxima atividade é resolver conflitos com uma branch nova baseada no commit atual de desenvolvimento, portando apenas diffs ainda não equivalentes; somente após build/testes/homologação da nova arquitetura, autorizar promoção de execução à `main`. Sem essas evidências, qualquer mesclagem automática das 13 branches poderia reintroduzir SDK e UI antigos e alterar o produto de modo imprevisível.
