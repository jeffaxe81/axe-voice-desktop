# Native release candidate — acceptance record

The default build remains the unavailable SDK-free GUI. `AXV_ENABLE_NATIVE_SIP=ON` adds an authenticated native candidate, requires the audited SDK gates plus TLS/media profile, and is packaged separately. It is not a production release.

## Implementation

- One Baresip/libre owner thread. Bounded UI command slot; callbacks use the existing selected-UA adapter and session-fenced mailbox. Native REGISTER_OK plus ua_isregistered are required for dialing. No log parsing or process-liveness registration inference.
- Masked session password, passed through account_set_auth_pass, never embedded in AOR/config. The UI clears its input after submission; owned password buffers are erased after provisioning/stop. Third-party SDK internals are not claimed to provide secure-memory erasure.
- Register, originate, answer and hangup. Stop before reconfiguring. Mute, hold and transfer remain disabled.
- Explicit UDP or TLS with mandatory CA PEM and server verification. TLS connections must not downgrade to plaintext. SIP TLS does not imply SRTP.
- G.711 and Windows default communications endpoints through WASAPI. Audio panel selections remain previews, not native route selection.
- Separate per-user candidate installer with matching OpenSSL DLLs and upstream licenses; unsigned distribution clearly identified.

## Automated evidence required before integration

- Existing default GUI/test suite plus native account validation tests.
- Real MSVC SDK compile/link; real owner reactor and native lifecycle.
- Local UDP Digest registration: accepted password, rejected password, timeout, restart and stop revocation.
- Local SIP TLS: trusted certificate, untrusted certificate, hostname mismatch. Certificates and test accounts generated locally, never real PBX secrets.
- Candidate GUI controls smoke and silent installer install/file-integrity/uninstall smoke.

## External acceptance still required

No central address, transport, trusted CA or access route has been provided for the user's Digitro PBX. This cloud environment has no configured VPN/TCP grants to the private PBX and no Windows physical headset. Therefore real PBX registration/calls and bidirectional RTP/headset acceptance cannot be claimed here. Enter credentials only inside the candidate app; do not add them to GitHub or logs.

Run on the target Windows PC: register the disposable test extension, reject an incorrect password, place and receive a test call, answer/hang up, verify speech in both directions, stop/restart, unplug/replug the headset, and verify the configured default communications endpoints. For TLS, use the PBX trust chain and verify that invalid/untrusted/wrong-host certificates fail. Record actual PBX model/version, transport, codec and whether media encryption is required. If SRTP is required, this initial candidate is not acceptable yet.

Status: implementation and CI validation in progress; pending external acceptance. Automated test evidence will be linked in the PR after execution.
