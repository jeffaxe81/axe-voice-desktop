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

 
## Native registration confirmation cross-check

The experimental adapter additionally calls the Baresip public
`ua_isregistered(selected_ua)` on `BEVENT_REGISTER_OK`, and before
accepting `BEVENT_CALL_INCOMING/OUTGOING`. An event claiming success
while the selected UA is not registered is **not** accepted as proof:
the callback epoch is revoked and all queued events are invalidated.
A locally successful `ua_register()` submission still leaves the
account pending until a valid native callback arrives.

Fake-SDK regression tests deliberately emit a success event while
`ua_isregistered()` is false, and attempt a call after simulated native
registration loss. Both fail closed. No claim of actual registrar
authentication, media readiness, TLS/SRTP, or working native Baresip
SDK linkage is made; the Windows UI remains unchanged.


## Revoked native registration during synchronous submission

The isolated gateway now checks `bridge.active()` immediately after
`ua_register()` returns, even if that function returns zero. Some callbacks
can synchronously revoke the native generation while the submission is in
progress. The gateway returns `NativeSessionRevoked` rather than retaining
a selected UA which may have been destroyed, or claiming an active request.

`stop()` skips `ua_unregister()` if native callback ownership has been
revoked, because a terminal UA shutdown may already have released that
native object. A second account cannot be submitted until the gateway is
explicitly stopped to clear its stale pointer. In the normal live path,
`ua_unregister()` still runs before the callback bridge is removed.

The fake-SDK tests cover synchronous UA shutdown, global exit, unverified
register-success, later shutdown, blocked reuse and successful recovery.
This protects local lifecycle assumptions but is **not a substitute** for
ownership, callback quiescence and UA lifetime guarantees from a real,
version-pinned Baresip event loop. Production SIP and audio remain disabled.


## Explicit registration deadline (lab only)

`BaresipSdkRegistrationGateway` now optionally arms an elapsed-time bound
through `arm_registration_deadline(monotonic_ms, max_wait_ms)`. The native
event-loop owner **must** call `poll_registration_deadline(monotonic_ms)` on
the same serialized thread. A local call to `ua_register` is never proof of
registration; a confirmed callback is observed only when an accepted
`BEVENT_REGISTER_OK` for the selected generation is followed by an actual
`ua_isregistered(selected_ua)` check.

Until a callback arrives the poll result is `Pending`. At the deadline,
on a rollback of the monotonic clock, or after a revoked native generation,
the gateway invokes controlled `stop()` and invalidates the event mailbox.
An early valid callback clears the deadline; a later attempt cannot inherit
it. Time arithmetic uses subtraction, preventing addition overflow. Tests
exercise edge timing, counter rollover, clock rollback, callback success,
revocation and recovery.

**The watchdog is opt-in and has no background timer:** without an owning
native event-loop providing a monotonic clock and polling, nothing expires.
This experimental code is not linked to AXEVoice and does not register with
a real PBX, confirm TLS/SRTP, or start audio media.


## Native event-loop thread ownership enforcement

The registration gateway now captures its owner thread on construction
(`BaresipNativeThreadGuard`). Calls to `submit`, `stop`,
`arm_registration_deadline` and `poll_registration_deadline` from
another thread are **rejected before touching the Baresip C SDK**.
The `submitted`/`session` queries also return false/zero off-owner
without reading the mutable native UA state. `stop()` now returns a bool:
false on wrong thread, true after owner-thread cleanup.

An unauthorized call never silently marshals to a worker, changes the
watchdog, unregisters callbacks, or retains an unknown UA. The owning
native event-loop must explicitly perform controlled shutdown; this guard
does not replace the lifecycle fence for callbacks or establish SIP proof.

Tests use a real C++ worker thread against the gateway, verify the
SDK-fake register/unregister counts stay unchanged, and confirm that the
deadline remains pending and the owner can stop afterward. The
`baresip_native_thread_guard` test checks thread affinity in isolation.

No native event-loop is launched here. This still is an experimental
adapter not linked into the GUI, not a SIP call implementation.
