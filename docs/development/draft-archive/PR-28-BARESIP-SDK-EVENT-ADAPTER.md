> **CÓPIA DOCUMENTAL HISTÓRICA DA PR #28.** Versão extraída da branch `feature/baresip-call-ownership-gate`. Não é registro de merge do código nem evidência atual de compilação/testes; o texto abaixo pode conter afirmações históricas que precisam de revalidação. A `main` não ganhou estas funcionalidades. Consulte a [matriz das 13 PRs](README.md).

# AXE Voice — experimental Baresip SDK event adapter (v0.39)

The optional bridge in `src/baresip_sdk_event_adapter.h` compiles against
Baresip's **public C API**:
`bevent_register`, `bevent_unregister`, `bevent_get_ua` and
`bevent_get_call`. The upstream callback signature uses
`bevent_h(enum bevent_ev, struct bevent*, void*)`.

Upstream declarations: https://github.com/baresip/baresip/blob/main/include/baresip.h

## v0.41 — isolating terminal events and failures

- `BEVENT_SHUTDOWN` is emitted for a **specific UA**. It only revokes the
  session when `bevent_get_ua(event)` matches the UA explicitly selected by
  AXE Voice. A shutdown from a different UA is ignored.
- `BEVENT_EXIT` represents global termination and revokes the selected
  session even without an associated UA event.
- `BEVENT_AUDIO_ERROR` and `BEVENT_SIPSESS_FAILED` from the selected UA
  conservatively revoke queued events and its unverified call state.
  Failures from other UAs are ignored. No information from these callbacks
  is interpreted as proof of a secure media stream or successful call.
- These changes are covered by the fake-SDK adapter regression test. The
  production GUI remains disconnected from the native adapter; no SIP login,
  registration, calls or audio media are implemented here.

## Deliberate limitations

- **Not linked into the AXEVoice GUI.** Its default engine remains
  `UnavailableSipEngine` and `SipEvidence::None`; no SIP registration,
  signaling, calls, RTP or SRTP is implemented in the application.
- The bridge translates only registration, unregistration, call start, call
  establishment, call closure and app shutdown. Other native events are ignored.
  Media security is never inferred from an established-call event.
- It binds to one explicit `struct ua*`, ignores callbacks from other UAs,
  supports one active call, and maps call-object identities to monotonic
  numeric tokens. No raw pointer, URI or credential enters the mailbox.
- Native call pointers are retained **in memory within the adapter only**.
  Pointer reuse within the same generation fails closed, as does a late event
  for a previous call during a new call.
- `bevent_register` is a global callback registry per handler. Only **one
  instance** of this bridge may run per embedded Baresip event loop.
- `start()`, callback dispatch and `stop()` require the SAME serialized
  event-loop owner. **Before `stop()` or destruction, the owner must establish
  a callback quiescence barrier.** The class alone cannot prevent an in-flight
  raw C callback from accessing freed resources.
- This class **does not attest callback provenance** and cannot issue
  `VerifiedNativeApi`. That must be provided by a separately audited
  lifecycle/ownership integration, after real SDK linkage and tests.

## Build

Windows CI builds `test_baresip_sdk_event_adapter` with the local fake
header in `tests/fakes/baresip.h`; it validates event translation and does
not run Baresip or access a SIP account.

To compile a separate experimental library against an audited real SDK, set
`AXV_BUILD_NATIVE_BARESIP_ADAPTER=ON`, and supply
`AXV_BARESIP_INCLUDE_DIR` and `AXV_BARESIP_LIBRARY`. No automatic SDK
download, network listener, credentials or GUI integration is enabled.

Before enabling production: pin and audit Baresip and libre versions, hashes
and licenses; implement an owned native event-loop with callback quiescence;
secure account credential handling; trust issuance; call controls; device
routing; TLS/SRTP verification; call/SIP integration tests and manual Windows
audio/headset QA.


## Chamada vinculada ao UA correto — proteção adicional

O adaptador exige duas verificações independentes em eventos de chamadas:
`bevent_get_ua(event) == selected_ua` **e**
`call_get_ua(bevent_get_call(event)) == selected_ua`.
Isso evita que um envelope de evento associado à conta selecionada aceite
uma chamada cujo objeto nativo pertença a outra conta. A inconsistência
revoga a sessão e descarta os eventos pendentes; o processo não volta a
receber estados SIP confiáveis sem uma nova inicialização controlada.
A regra também é aplicada a estabelecimento e encerramento de chamadas.

A suíte fake-SDK inclui cenários de propriedade divergente no início e
na continuação da chamada, mais regressão de uma chamada válida após
reiniciar. A verificação usa a API pública `call_get_ua()` do Baresip.
**Não significa que a GUI esteja vinculada ao SDK real**, nem habilita
registro SIP, chamadas ou RTP/SRTP.
