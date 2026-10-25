# Native release candidate — acceptance record

The default build remains the unavailable SDK-free GUI. `AXV_ENABLE_NATIVE_SIP=ON` adds an authenticated native candidate, requires the audited SDK gates plus TLS/media profile, and is packaged separately. It is not a production release.

## Implementation

- One Baresip/libre owner thread. Bounded UI command slot; callbacks use the existing selected-UA adapter and session-fenced mailbox. Native REGISTER_OK plus ua_isregistered are required for dialing. No log parsing or process-liveness registration inference.
- Masked session password, passed through account_set_auth_pass, never embedded in AOR/config. The UI clears its input after submission; owned password buffers are erased after provisioning/stop. Before UA destruction the pinned SDK account password allocation is explicitly wiped, including its shared Digest references; a real SDK test checks a retained reference. Other third-party transient copies are not claimed to provide secure-memory erasure.
- Register, originate, answer and hangup. Stop before reconfiguring. Mute, hold and transfer remain disabled.
- Explicit UDP or TLS with mandatory CA PEM and server verification. TLS connections must not downgrade to plaintext. SIP TLS does not imply SRTP.
- G.711 and Windows default communications endpoints through WASAPI. Audio panel selections remain previews, not native route selection.
- Separate per-user candidate installer with matching OpenSSL DLLs, app-local MSVC runtime and upstream licenses; unsigned distribution clearly identified.

## Automated evidence required before integration

- Existing default GUI/test suite plus native account validation tests.
- Real MSVC SDK compile/link; real owner reactor and native lifecycle.
- Local UDP Digest registration: accepted password, rejected password, timeout, restart and stop revocation.
- Local SIP TLS: trusted certificate, untrusted certificate, hostname mismatch. Certificates and test accounts generated locally, never real PBX secrets.
- Candidate GUI controls smoke and silent installer install/file-integrity/uninstall smoke.

## External acceptance still required

No central address, transport, trusted CA or access route has been provided for the user's Digitro PBX. This cloud environment has no configured VPN/TCP grants to the private PBX and no Windows physical headset. Therefore real PBX registration/calls and bidirectional RTP/headset acceptance cannot be claimed here. Enter credentials only inside the candidate app; do not add them to GitHub or logs.

Run on the target Windows PC: register the disposable test extension, reject an incorrect password, place and receive a test call, answer/hang up, verify speech in both directions, stop/restart, unplug/replug the headset, and verify the configured default communications endpoints. For TLS, use the PBX trust chain and verify that invalid/untrusted/wrong-host certificates fail. Record actual PBX model/version, transport, codec and whether media encryption is required. If SRTP is required, this initial candidate is not acceptable yet.

## Executed evidence (25/10/2026)

At code SHA `e977367dca13f444cf2b2180168c9422f493145d`, [Windows CI40191814864](https://github.com/jeffaxe81/axe-voice-desktop/actions/runs/40191814864) passed70/70 tests (41 unit,2 blackbox,27 integration). [Native CI40191814868](https://github.com/jeffaxe81/axe-voice-desktop/actions/runs/40191814868) passed both SDK profiles, native secret/lifecycle/reactor probes, all six real Windows SIP Digest/TLS scenarios, native GUI smoke, and candidate installer install/file-integrity/uninstall smoke.

[Download the tested candidate installer artifact](https://github.com/jeffaxe81/axe-voice-desktop/actions/runs/40191814868/artifacts/12598860587). Extract the ZIP and run `AXE-Voice-Native-Candidate-Setup.exe`; GitHub may require sign-in. Installer SHA256: `683704fd3c350e936d26649fc79325d2784f08c357dd5ab527312426f42a8c70`.

An independent review found refresh-state handling, stale call commands, weak negative TLS test conditions and SDK password-retention issues. They were corrected with regression tests: short-expiry registration, session/call identity checks, valid credentials plus no REGISTER accepted by invalid TLS peers, and zeroed retained SDK password references. A temporary insecure TLS mutant failed the hostname test as intended. Upstream automatically omits loopback interfaces; explicitly selecting127.0.0.1 for local accounts resolved Windows WSAEADDRNOTAVAIL10049.

Status: automated candidate validation passed; real Digitro, bidirectional RTP/headset and production acceptance remain pending. This artifact is a native candidate; the default general workflow executable remains SDK-free.
