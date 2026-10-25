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

## v0.48 — SIP-message event origin isolation

Upstream `baresip/src/ua.c` emits `BEVENT_SIPSESS_CONN` and
`BEVENT_SIPSESS_FAILED` via `bevent_sip_msg_emit`, not via a UA
or established call. An unauthenticated or rejected inbound INVITE may cause
such events even when no owned UA is selected. They **must not** be
misclassified as events from the active account merely because they share the
process event registry.

The adapter now filters all unrecognized and message-only event kinds
*before* invoking `bevent_get_ua`. Only an allowlist of UA and call events
is eligible for ownership lookup and event translation. `BEVENT_EXIT`
remains global and fail-closed. A selected UA's `BEVENT_AUDIO_ERROR`
remains fail-closed. A SIP-message failure alone no longer revokes an
unrelated live UA session. `BEVENT_CALL_RTPESTAB` and `BEVENT_CALL_MENC`
are ignored and never treated as proof of RTP/SRTP safety.

The fake-SDK regression checks that message-only and unsupported events
produce **zero UA-identity lookups and zero mailbox events**, even with a
forged UA-like fixture. Registration events remain accepted for the owner.
No real Baresip SDK or SIP endpoint is executed by this test.

Sources (reconfirm against the pinned SDK tag before production):
- https://github.com/baresip/baresip/blob/main/src/ua.c
- https://github.com/baresip/baresip/blob/main/include/baresip.h

## v0.49 — exclusive owner of Baresip event handler registration

The upstream `src/bevent.c` `bevent_register(handler, arg)` calls
`bevent_unregister(handler)` first; the registry keys its subscription
by the **callback function pointer**, not by its `void* arg` context.
All AXE Voice adapters use one static callback function. Starting a
second instance could silently remove the first instance's subscription.

The optional adapter now uses an atomic single-owner claim before entering
the native SDK registration path. A second active instance is rejected
without touching the Baresip event registry. The claim remains held after
a callback rejects a session until the owning management thread invokes
`stop()`, unregisters the native handler and waits for the callback
lifecycle to become quiescent. SDK registration failure releases the claim,
allowing another instance to start. Stopping an instance that never owned
the subscription cannot release the actual owner's claim.

This is **not** a native callback resource-lifetime guarantee: the
Baresip event-loop owner still must serialize registration, callbacks
and shutdown, establish quiescence before destroying SDK resources, and
keep the adapter alive through that barrier. No actual Baresip runtime
or SIP account is started by the default Windows CI.

`baresip_sdk_single_owner` uses a fake registry with unique handlers
and tests duplicate startup, foreign `stop()`, fail-closed termination,
failed registration, recovery, and 64 alternating owner restarts.

Upstream implementation:
https://github.com/baresip/baresip/blob/main/src/bevent.c

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

## v0.44 — session-epoch isolation and lifecycle close/wait

The experimental SDK bridge now connects to `BaresipCallbackLifecycle`.
Each successful native callback registration has a **unique, immutable**
`CallbackContext` that captures both the fence generation and mailbox
session. An event originating from an older registration is silently rejected
even after the bridge restarts. A bounded set of up to 512 contexts remains
alive through restarts of the same adapter instance.

On a malformed call callback, the new `reject_current()` immediately closes
admission and discards events without blocking on its own callback, avoiding a
self-deadlock. `stop()` unregisters the SDK handler and waits for admitted
lifecycle operations to finish.

IMPORTANT: None of this alone guarantees that the upstream SDK has finished
all raw C callbacks. The future event-loop owner **must serialize native
registration, dispatch, unregistration, and destruction** and independently
establish callback quiescence. Do not destroy the adapter while any external
native callbacks may still reference old contexts. The bridge does not grant
`VerifiedNativeApi`, implement credentials/call operations, or link Baresip
into the AXEVoice GUI.

CTest `baresip_sdk_epochs` simulates stale callbacks after restart and
performs 100 additional start/stop iterations, using only fake SDK events.

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


## Cross-thread command mailbox and owner-only dispatcher (lab)

The GUI-side producer can enqueue `SubmitSelectedAccount` (bounded timeout)
or `Cancel` using a monotonically increasing command generation. Commands
contain **no UA pointers, account secrets, phone numbers or SIP URLs**. The
native-loop owner alone resolves a selected `ua*`, constructs
`BaresipSdkRegistrationGateway` on its own thread and calls
`BaresipRegistrationCommandDispatcher::tick()` with a monotonic clock.
The mailbox is a bounded 16-command queue: overflow discards the ENTIRE
pending batch, closes admission and directs the owner to stop SIP safely.
Cancel supersedes queued registrations, closes the generation and prevents
further submissions; a new generation cannot begin until cancellation or
overflow is drained.

The dispatcher rejects execution on a foreign thread, calls the native
registration gateway on the owner thread only, arms its deadline after
a successful local submission, and polls the deadline every loop iteration
(including iterations with no new commands). Expired, clock-regressed or
revoked generations force stop and command invalidation. A normal callback
confirmation is still required; a locally successful `ua_register` does
NOT grant `VerifiedNativeApi` or authorize a call.

This is still a **laboratory control-plane seam**, not a running Baresip
loop or a wired UI component. The owner must externally guarantee SDK
callback lifetime and serial event-loop operations. The fake-SDK tests
exercise cross-thread enqueue, wrong-thread execution, expiry, native
confirmation, cancellation, restart and overflow. Production SIP, TLS/SRTP
and audio remain intentionally disabled.


## Cancel/overflow during in-flight ua_register (fake SDK)

A UI request can cancel a command AFTER the event loop drains a Submit
but BEFORE the native `ua_register` call returns. The bounded queue
therefore carries a revocation revision with each drained batch. Cancellation,
overflow and explicit invalidation advance that revision.

The native dispatcher checks admission before and after `ua_register`,
and again after arming the watchdog. When a newer cancellation or overflow
supersedes a drained request, the dispatcher immediately stops the gateway,
drops pending SIP evidence, invalidates the command generation and reports
`SupersededDuringDispatch` rather than `Submitted`.

No mutex is held across native Baresip calls; callbacks cannot deadlock
against UI cancellation. The new mock SDK regression test injects actual
cross-thread cancellation/overflow **inside** `ua_register`; neither
case may leave a registered callback or usable SIP state behind.

This is a *reconciliation guarantee at owner-thread checkpoints*, not an
instantaneous inter-thread cancellation of a blocking SDK function.
Cancel requests arriving after the final checkpoint are processed by the
next event-loop tick; an external loop still must run and own native SDK
resources. Not a production SIP call or RTP implementation.


## Laboratory worker with dedicated SIP owner thread

`BaresipRegistrationLabWorker` now creates the experimental adapter,
lifecycle, gateway and dispatcher **inside a dedicated C++ thread**, and
joins only after native callback unregistration and queue invalidation.
Its public `submit()` and `cancel()` contain no UA pointers, credentials
or SIP URIs. A mutex-protected snapshot publishes the latest significant
state and a monotonic generation to external observers.

The test-only pump, UA resolver and optional virtual clock run exclusively
on the worker thread. The fake-SDK test checks thread identity, successful
callback confirmation, cancellation, timeout, Baresip shutdown and restart
with a fresh generation.

**Important:** this is a lab-thread scaffold, not an actual libre
`re_main()` event loop, audited SDK initialization, account provisioning,
real SIP registration, GUI integration or voice media transport. The caller
must serialize start/stop, keep the UA alive through shutdown and establish
native callback quiescence before destroying the real SDK.


## Audited native SDK compile/link gate

The optional build now requires pinned `baresip.h`/`re.h` and their
MSVC import libraries. Upstream Baresip's public API uses libre, and
the recommended multi-threaded integration is to post to a message
queue owned by the `re_main` thread. The C++ ABI contract checks
the function pointer types; an optional real-library executable link
probe catches unresolved imports. Default Windows CI remains offline and
uses test-only headers; nothing is connected to the GUI yet.


## Real libre event-loop host — optional, no SIP

`BaresipNativeLibreReactor` creates a dedicated native-loop owner thread.
The thread calls the audited libre API `libre_init()`,
`mqueue_alloc()`, `tmr_init()`, periodic `tmr_start()`, and
`re_main()`. A foreign thread may only `wake()` via
`mqueue_push()`. Its native message handler and periodic timer invoke
owner-thread hooks; stop requests are delivered through the native
queue and the timer can also call `re_cancel()` on the owner.
Native queue/timer cleanup and `libre_close()` run on that thread.

An offline fake-libre test verifies owner identity, cross-thread wake,
timer-driven polling, shutdown/restart, and initialization failures.
When **AXV_BUILD_NATIVE_BARESIP_ADAPTER=ON** with audited/pinned
libraries, the separate `axv_native_libre_reactor_smoke` binary
compiles and runs against the REAL libre `re_main` with no account
creation or SIP traffic.

**Scope:** this is the actual libre reactor primitive, NOT initialization
of the Baresip core, a provisioned UA, or the registration dispatcher
wired into that core. The UI remains disconnected, and no call/RTP
features are enabled. The default Windows CI uses fake libre.


## Register dispatcher running on libre's owner thread

`BaresipNativeRegistrationHost` is the first wiring of the bounded
registration command queue and owner-thread gateway to
`BaresipNativeLibreReactor`. At `on_start`, the reactor thread
constructs the event mailbox, lifecycle, adapter, gateway and command
dispatcher, then publishes a new opaque generation. A GUI or controller
thread may only enqueue `submit(timeout_ms)` or `cancel()`;
`mqueue_push` wakes `re_main()` for owner-side processing and
the timer invokes polling even if a wake is missed.

A rejected wake forces a queued cancellation rather than leaving
an unacknowledged request pending. Shutdown stops admission, invalidates
the generation and **joins after the owner thread unregisters**
callbacks. Restart requires a fresh native control-plane generation.
A diagnostic snapshot exposes only outcomes, ticks and fault state,
never credentials, SIP URI, call object or `ua*` pointer.

The new default CI fake-libre + fake-Baresip integration test verifies
UI-to-native register routing, native confirmation, cancellation,
deadline expiry, terminal native shutdown, failed wakes and failed
initialization across restarts. It is a **control-plane seam only**:
the caller must still initialize Baresip itself, securely provision/
own the UA and keep it alive throughout `stop()`. The actual
`AXV_BUILD_NATIVE_BARESIP_ADAPTER=ON` SDK gate is separate.
No real PABX login, RTP, SRTP, GUI dialing or voice is enabled.
