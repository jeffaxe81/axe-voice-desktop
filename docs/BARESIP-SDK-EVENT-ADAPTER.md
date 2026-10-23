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

## Callback epochs and lifecycle integration (parallel workstream)

`BaresipSdkEventAdapter` now uses `BaresipCallbackLifecycle`. Each
`bevent_register` receives a distinct immutable callback context (fence
generation + mailbox session), retained until adapter destruction. A delayed
callback from an older registration cannot be mistaken for a current callback.

On malformed, terminal or wrong-call events, `reject_current()` closes the
admission gate and invalidates the mailbox **without waiting in the callback**.
At normal stop, the adapter unregisters the SDK handler and waits for admitted
lifecycle work. The number of retained contexts is bounded to 512.

**Critical external requirement:** only a serialized Baresip event-loop owner
can make C API callback dispatch, start, and stop safe. Before adapter
destruction it must guarantee a native callback quiescence barrier. The fence
does not make late callbacks into a destroyed object safe. No SIP/media
functionality or VerifiedNativeApi token is implemented in this workstream.

`baresip_sdk_callback_epochs` (mock SDK) checks stale callback dispatch
after stop/start and 100 consecutive restarts. This workstream deliberately
does not bump the application's version until integration into the main lab PR.

## Exclusividade por processo do callback C

O bevent_unregister recebe um ponteiro de funcao, sem contexto. Dois
adaptadores poderiam cancelar reciprocamente o registro. Uma reserva
global atomica garante somente um adaptador ativo com esse callback.
start() recusa competidores; uma falha de lifecycle.begin ou de
bevent_register libera a reserva. stop() so libera depois de
bevent_unregister e quiescencia. A reserva permanece ativa apos
falhas internas ate que stop() descarte o registro nativo.

Teste: duas instancias independentes, disputa, falha de registro e
reutilizacao segura apos stop(). A reserva nao substitui o loop nativo
serializado, nao garante SIP real nem autoriza a interface para chamadas.


## Experimental native UA registration submission

The isolated `src/baresip_sdk_registration_gateway.h` installs the native
event bridge **before** invoking `ua_register(selected_ua)`. A zero return
means only *submitted to the Baresip SDK*, NOT authenticated or registered.
The application must wait for a trustworthy `BEVENT_REGISTER_OK`, bound to
the selected UA and current callback generation, before considering the SIP
account registered. This gateway does not issue `VerifiedNativeApi`.

The selected UA must be securely provisioned beforehand by the serialized
Baresip event-loop owner, and must remain alive until `stop()` completes.
On errors from `bevent_register` or `ua_register`, the gateway revokes
the bridge and pending events. `stop()` submits `ua_unregister` and
removes the callback. The fake-SDK unit test checks rejected submissions,
register-pending vs. confirmed, duplicate attempts, logout and restarts.
Neither this gateway nor its SDK fake is linked into the GUI.
