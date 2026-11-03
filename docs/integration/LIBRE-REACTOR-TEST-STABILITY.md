# AXE Voice v0.80 — estabilidade dos testes do reator libre

A v0.80 melhora a confiabilidade e o diagnóstico do teste
`libre_owner_reactor` no Windows. Não altera os mecanismos
de áudio, SIP, autenticação ou execução nativa do produto.

## Ocorrência

Na CI pós-merge da v0.78, o teste passou na execução geral de
127 testes e falhou na repetição do grupo de integração (46/47).
A reexecução do mesmo job no mesmo commit passou.

Essa é evidência de intermitência. **A causa raiz ainda não foi
comprovada**; acompanhamento na issue #169.

## Mudanças nos testes

- A verificação de primeiro callback não depende mais de uma espera
  fixa de 30 ms; o teste aguarda o incremento real do contador de ticks,
  com prazo máximo de 1 segundo.
- A verificação de falha controlada no callback não depende mais de
  esperar 20 ms; exige a publicação observada do estado `HookFailed`.
- Todas as demais exigências continuam ativas, incluindo propriedade
  de thread, inicialização/fechamento de recursos, proteção contra
  falhas de fila, retomada e detecção de loop nativo com erro.
- Se algum estado não ocorrer no prazo, o teste **falha** e imprime
  código do cenário e contadores internos. Não há retries que ocultem
  erro do próprio teste.
- O gate `integration` também roda o `libre_owner_reactor` por 10
  repetições com `ctest --repeat until-fail:10`; qualquer repetição
  não aprovada reprova o pipeline.
- Isso fortalece a observabilidade e a aceitação, sem substituir
  homologação em Baresip real, central SIP ou headset.

## Condições para integração

Exigir CI Windows, SDK nativo e integridade da baseline aprovados
no SHA exato da PR. A issue #169 deve continuar aberta até haver
evidência de estabilidade em execuções independentes da CI e
identificação da causa que provocou a falha anterior.
