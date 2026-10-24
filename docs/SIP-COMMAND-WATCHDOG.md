# AXE Voice — SIP command timeout and watchdog (v0.51)

This applies to the **experimental** `SipCommandExecutor` and the optional
Baresip native call port. These components are **not linked into the GUI**.
The desktop application still uses `UnavailableSipEngine` and
`SipEvidence::None`; no real registration, calls or RTP/SRTP exist.

## Purpose

A successful `ua_connect`, `ua_answer` or `ua_hangup` submission does NOT
guarantee that the corresponding state-change callback will arrive. An
unbounded `PendingTransition` could otherwise leave commands stuck or tempt
the operator to retry an action that already had native side effects.

`SipCommandExecutor` now requires a nonzero **monotonic milliseconds** value
for every `dispatch(..., now_ms)`. The SIP event-loop owner must sample it
immediately before dispatch, using a platform monotonic source (e.g.
`std::chrono::steady_clock`), **never wall-clock time or a UI timestamp**.

After a native port accepts an action, it has **10,000 ms** to observe progress:
a change in SIP status, active call token or last-call token. Two ways to
enforce the deadline:

- `watchdog(live, active_session, now_ms)` — poll on the serialized SDK owner
  loop even if there is no new user command.
- `dispatch(..., now_ms)` — performs the same deadline check before
  attempting a new native command in that generation.

At the exact deadline with no progress, return `CommandTimedOut` and fence
the executor. Further commands in that session return `Fenced` regardless
of later callbacks; a *new SDK/UA session generation* is required to recover.
A monotonic clock rollback returns `ClockRollback` and fences. A missing
or invalid session/provenance on a pending command returns `StaleSession`
and fences.

Native errors and C++ exceptions also fence as before. The watchdog never
issues `ua_hangup`, retries an INVITE, logs a SIP URI/credential, or creates
`VerifiedNativeApi`. The owning integration must independently invalidate
the native callback/session when watchdog fencing occurs, obtain actual
callback quiescence and then create a fresh session; it must not simply
reset the executor while keeping the same UA.

## What counts as progress and what is not proven

Any change in the projected SIP state or opaque call tokens clears the
single pending command. This detects movement through the event stream; it
does **not** prove media security, audio quality, call completion or remote
answer. A production engine should additionally define per-action terminal
states, a mapping from native submission to Baresip events, transaction
timeouts and explicit cancellation policy after real PBX tests.

The owner must serialize `dispatch`, `watchdog`, Baresip callbacks and
native port execution. The executor itself is NOT thread-safe.

## Windows CI verification

`test_sip_command_watchdog` exercises the synthetic Baresip-engine and
native-port boundary: exact 10-second expiry, no resend after expiry,
registration-proof loss, progress clearing a pending action, clock rollback,
stale session rejection, and recovery only with a new generation. The
existing `test_sip_command_executor` also supplies monotonic test ticks.

These tests are deterministic simulations. They do not launch a real
Baresip SDK, place calls or test Windows audio hardware.
