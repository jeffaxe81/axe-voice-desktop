# Native audio endpoint routing

Next recorded release item: connect existing microphone/playback selections to the native WASAPI call path. The existing Windows/C++17/Baresip4.12 candidate remains the platform. Apply selections at registration and retain copied endpoint IDs for that session. Empty selections mean Windows default communications devices. Ringtone preview remains independent.

Chosen implementation: in-memory Baresip audio_source/audio_player configuration from validated opaque endpoint IDs; a fresh Core Audio inventory on the native owner thread before originate/answer rejects missing selected endpoints. Hangup remains available. Live mid-call route switching is deferred because safe media restart/quiescence is not yet validated. A UI-only selection would leave the recorded release gap unresolved.

IDs are never logged or persisted; reject config delimiters/control/Unicode and lengths above the SDK127-byte field. Registration itself remains available without audio hardware. SIP callback evidence does not imply media proof. A rejected audio preflight leaves registration intact and reports a device error. Active-session microphone/playback selectors stay locked; refreshing inventory preserves an explicitly selected missing ID rather than replacing it with another device. Stop/re-register to change routes.

Evidence: portable route validation/presence/fixed-selection tests; real Baresip config parser verifies exact source/playback IDs and default mapping; native engine snapshot/smoke; existing Windows70-test suite and Digest/TLS scenarios; native GUI smoke plus installer install/uninstall. No physical audio or Digitro interoperability claim. Default SDK-free GUI remains unchanged.
