# AXE Voice — isolated Baresip native call-command port (v0.50)

The opt-in C++ adapter in `src/baresip_sdk_call_port.h` implements
`SipNativeCommandPort` using the public Baresip C API:

- `ua_connect(ua, nullptr, nullptr, uri, VIDMODE_OFF)` — outgoing voice only
- `ua_answer(ua, owned_call, VIDMODE_OFF)` — incoming voice answer
- `ua_hangup(ua, owned_call, 0, nullptr)` — active-call hangup

This is a **laboratory source component**. It is deliberately NOT linked
into `AXE-Voice.exe` and does not create accounts, sockets, SIP registrations,
credentials, RTP, SRTP, or headset audio. The default Windows CI links and
executes mock Baresip API functions, not a real SDK.

## Safety conditions

- A command-port instance captures the **exact mailbox session** on
  construction; a port from before a restart can never target the new
  generation. Construct it on the serialized Baresip event-loop thread.
- All native commands also check the calling thread, adapter activity,
  current UA ownership and the latest SDK-reported registration state.
  `sip_registered_` is **not** `SipEvidence::VerifiedNativeApi` and must
  never enable the UI or be inferred from logs/process status.
- Every callback supplies its **real opaque mailbox token** through
  `BaresipCallbackIngress` and `BaresipCallbackLifecycle`. The port checks
  that token before retrieving the current native call pointer; it never
  derives a pointer from an ID, stores it in the mailbox or serializes it.
- Only an **incoming, non-established** call can be answered. Repeated answer
  or hangup commands are blocked even before an asynchronous callback arrives.
  The SIP command executor additionally prevents repeated/stale request IDs.
- No saved pointer is used after calling `ua_answer` or `ua_hangup`; the
  SDK can reenter and close/free calls synchronously.
- Outgoing destinations must be validated, printable ASCII `sip:`/`sips:`
  URIs of at most 512 characters. Bare extensions and internationalized
  destinations remain unsupported by this native experiment until a
  trusted account-aware URI resolver is implemented.
- A failed outgoing call must be fenced at the `SipCommandExecutor` owner:
  a native failure may have partial side effects. Never transparently
  resend it as a retry inside this port.
- `BaresipSdkEventAdapter::stop()` still requires external native-event-loop
  quiescence and must occur before SDK resources are destroyed.

## Automated tests and real-SDK gates

`test_baresip_sdk_call_port` uses a fake SDK and synchronous callbacks to
validate missing registration, invalid/Unicode destination, outgoing call,
incoming answer, established state, hangup, repeat commands, stale tokens,
wrong thread, shutdown, restart and a native call failure. The native ABI
linker probe now also validates the exported `ua_connect`, `ua_answer`
and `ua_hangup` symbols when a genuine SDK is optionally configured with
the SHA-256 requirements.

**Still missing for actual telephony:** pinned and audited Baresip/libre
SDK for Windows; native event-loop lifetime and callback ownership proof;
a real `SipEngine` that supplies verified registration; account secrets and
provisioning; PBX test calls; bidirectional headset media; TLS/SRTP validation,
error recovery and production signing/QA. Do not release or merge this PR
until these gates pass.

Upstream API:
https://github.com/baresip/baresip/blob/main/include/baresip.h
