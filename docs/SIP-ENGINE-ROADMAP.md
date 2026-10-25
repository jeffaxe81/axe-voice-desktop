# SIP engine boundary

The `SipEngine` interface is the contract for SIP registration and voice calls. The default `UnavailableSipEngine` is intentionally non-operational: it cannot register or dial. An optional native candidate now implements the interface; see [candidate acceptance](integration/NATIVE-RELEASE-CANDIDATE.md). UI messages must not report success.

The candidate uses pinned Baresip/re4.12 and OpenSSL3.5.7, session-only passwords and native callback snapshots. Automated local Digest/TLS and installer checks pass. Candidate0.52 adds selected WASAPI capture/playback routes fixed at registration, with missing-device action refusal. Candidate0.53 adds SDK microphone mute and local hold/resume requests with call/session-bound readback; local hold is not remote confirmation. Candidate0.54 adds a one-attempt direct transfer request with destination review and call/session binding, plus call-feedback cleanup. Submission is not confirmed transfer success. Next: real PBX registration/calls and physical bidirectional audio/headset acceptance, validated live route migration, and SRTP if required by the deployment. Keep logs free of secrets.

No passwords or tokens belong in source code or unencrypted configuration. A built GUI alone is not evidence of registration or media flow.
