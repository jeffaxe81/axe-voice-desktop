> **ARQUIVO HISTÓRICO — PR #39 em rascunho (24/10/2026).** Este documento foi recuperado da branch `feature/sip-fail-closed-regressions-v045` e publicado na `main` apenas para rastreabilidade. O código correspondente **não foi mesclado nem compilado ou testado nesta rodada**. Declarações sobre testes no texto original são registros históricos, não evidência atual. Ver [auditoria de PRs](README.md).

# AXE Voice — regressões fail-closed (v0.44)

## Objetivo

Adicionar duas barreiras verificáveis à branch Windows de laboratório,
sem habilitar o backend Baresip real:

1. `sip_batch_bounds` injeta lotes sintéticos com comprimento maior que
   o array de 32 eventos e com `std::size_t` máximo. Ambos devem revogar
   a geração antes de acessar o array; eventos posteriores na mesma sessão
   permanecem bloqueados. Uma geração realmente nova poderá reiniciar.
2. `sip_dial_matrix` percorre **6.144 combinações determinísticas** de
   processo, evidência, estado SIP, geração, tipo de engine, status da engine
   e destino. Exige todas as condições simultâneas para retorno positivo
   de `authorize_dial`; confirma também que o engine realmente criado
   pela UI, `UnavailableSipEngine`, **nunca habilita chamadas**,
   mesmo com estado registrado forjado.

## Limite de evidência

O teste usa `SipEvidence::VerifiedNativeApi` APENAS como dado sintético
do teste. Isto não cria nem autentica um callback real. O CTest Windows
verifica regras de segurança do software; não valida sinalização SIP,
chamadas, RTP/SRTP, codec, headset ou compatibilidade com PABX.

## Critérios de aceite

- Compilar no runner Windows 2022 (MSVC) em C++17.
- Executar todas as categorias `unit`, `blackbox` e `integration`.
- Confirmar os dois novos testes nas listas CTest.
- Não avançar a `main` para produção sem SDK, teste funcional real,
  ciclo de vida auditado, TLS/SRTP e aprovação de homologação.
