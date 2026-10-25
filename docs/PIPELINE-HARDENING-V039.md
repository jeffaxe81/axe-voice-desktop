# AXE Voice Desktop v0.39 — pipeline de eventos (laboratório)

## Por que esta branch existe

A branch de bootstrap já contém `BaresipCallbackIngress`, com sequências
monotônicas de chamada e teste de integração no `test_baresip_callback_ingress`.
O PR #15 introduziu, em paralelo, outro ingresso de eventos e divergiu após
novos commits no bootstrap; o PR #17 foi empilhado sobre ele. Esta entrega
reaproveita **o componente mais recente da branch-base**, sem fazer force-push
ou mesclar os protótipos sobrepostos.

## Mudanças

- `BaresipCallbackIngress` passa a exigir identidade opaca de conta
  validada **externamente** em `begin_session` e a mesma conta em todo
  callback de registro/chamada. Divergência revoga a sessão, enquanto
  callbacks de geração antiga são descartados sem afetar a geração nova.
  Esta verificação NÃO autentica o chamador por conta própria.
- `SipEventProjection::observe` rejeita `Batch.size` superior ao tamanho
  físico da fila, antes de iterar os eventos. O bloqueio da geração é definitivo.
- Novo CTest `baresip_pipeline_hardening` exercita
  `BaresipCallbackIngress` → `BaresipEventMailbox` →
  `SipEventProjection` para registro/chamada/encerramento simulados,
  proveniência ausente, lote corrompido, replay de geração, revogação e
  concorrência de callbacks.
- Nenhum callback nativo foi ligado. O teste injeta
  `SipEvidence::VerifiedNativeApi` artificialmente SOMENTE no processo do
  teste; o executável permanece com evidência `None` e
  `UnavailableSipEngine`, sem chamada ativa.

## Portões para homologação

1. Pin de versões/hashes/licenças de Baresip, libre/re e codecs.
2. Adaptador nativo in-process auditado, capaz de atestar fonte e geração.
3. Remover callbacks antes do teardown; sincronizar workers de forma explícita.
4. Verificar registro SIP, originação e recepção com PABX real.
5. Validar áudio bidirecional e hotplug de headset, TLS/SRTP,
   Windows 10/11, instalador e empacotamento.
6. Exigir CI Windows/CTest do novo PR antes de qualquer merge.

**Não afirmar registro SIP nem liberar chamadas reais com testes sintéticos.**
As regras de isolamento por conta do PR #15 foram incorporadas ao adaptador
já existente da base. O adaptador SDK e a camada de ciclo de vida agora propagam a identidade da UA vinculada à geração. Os PRs #15 e #17 continuam abertos em Draft até revisão
formal das diferenças, sem merge automático nem force-push.
