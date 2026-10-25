# Candidate0.54 — call feedback cleanup and direct transfer

Continue the recorded call controls. First clear call-scoped audio/hold/mute feedback when the established call closes or its identity changes, preserving errors for an unchanged call and no-call device refusal.

Chosen transfer scope: direct/blind SIP REFER using pinned Baresip call_transfer, one request attempt per selected established call. Attended/consultative transfer needs a multi-call model and is deferred. Commands carry the displayed native session/call identity; target validation and TLS no-downgrade use the configured account, never changed UI account fields. Reject controls, config/header separators, URI headers/escapes, unsupported parameters and oversized targets. Revalidate at the native port before consuming its one-attempt guard.

The GUI gets a dedicated Transfer button beside Call and reuses the existing destination field. A destination review dialog precedes sending; the captured call snapshot remains fixed while it is open. Confirmation after call replacement is rejected, never retargeted. Default SDK-free GUI remains unchanged.

SDK return0 is only local submission. Media snapshot exposes None/Submitted/Failed; selected-UA/call BEVENT_CALL_TRANSFER_FAILED marks failure without ending another call or revoking registration. Never automatically hang up after sending; upstream may close only after its actual successful transfer notification. Do not label generic call closure as confirmed transfer success. A failed attempt remains consumed until a new call; hangup/mute/hold stay available.

Validation: regression tests for feedback lifetime; transfer URI/TLS bounds, session/call policy and one-attempt admission; native SDK port fixtures for invalid/stale/thread/duplicate/reentrant close and foreign/selected failure events; real SDK ABI type/link; no-call/UI smoke; full default Windows suite and existing7 native/installer scenarios. Actual Digitro REFER/NOTIFY interoperability and transferred media remain external acceptance. Candidate only, no production or SRTP claim.
