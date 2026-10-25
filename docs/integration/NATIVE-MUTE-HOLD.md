# Native candidate0.53 — microphone mute and local hold/resume

The native call row now exposes Answer, Hold/Resume, Microphone mute/unmute and Hangup. Controls require an established, callback-verified call and a current pointer-free SDK media snapshot. UI intent carries the exact displayed session/call identity; a delayed click cannot be silently retargeted to a replacement call. Default SDK-free controls remain disabled; transfer is not implemented.

The native owner resolves the selected call and invokes audio_mute or call_hold, then reads audio_ismuted/call_is_onhold. Boolean desired states are idempotent; stale token/session, wrong thread, non-established calls and calls already ending are rejected. Closing/stopping or changing call identity clears media state and prior control errors.

Pinned Baresip4.12 changes its LOCAL on_hold flag before modifying SDP and may return an error after that change. The candidate therefore reports a local hold request, not confirmed peer/PBX acceptance. Immediate errors remain visible while readback still reflects the actual SDK local flag. Hangup remains available. Resume requires a fresh audio-route inventory; putting on local hold/muting/hanging up do not depend on a present device.

Validation gates: session/media-currentness and audio-preflight policy; SDK port fixtures for ownership/thread/stale/duplicate controls and local mutation before hold error; compile-time and link ABI checks for all consumed control symbols; real SDK audio-object mute/unmute readback with RTP disabled and no source/player started; native no-call rejection and GUI smoke; full Windows suite, seven native registration/TLS/audio-refusal scenarios, and installer install/uninstall. The real mute probe does not open a microphone or prove acoustic silence; SDK port fixtures do not prove peer hold negotiation.

Actual Digitro call control, audible microphone silence, SIP hold/resume interoperability and physical headset behavior remain external acceptance. No SRTP, transfer, production signing or automatic live route migration is claimed. Each candidate PR links its exact-head installer artifact and executed evidence.
