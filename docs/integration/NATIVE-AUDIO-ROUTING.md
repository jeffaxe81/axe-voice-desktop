# Native candidate0.52 — capture/playback endpoint routing

The release item "native endpoint routing" now connects the existing Audio panel's microphone and voice selections to Baresip's WASAPI source/player configuration. Registration captures a copied in-memory route. No explicit selection means the Windows default communications endpoint; an explicit selection remains bound to that ID. Stop the native engine before changing it. Ringtone selection remains an independent local preview.

The native owner queries a fresh Core Audio inventory immediately before originate/answer. Missing capture/playback endpoints or inventory failure reject that action while leaving registration intact; hangup is independent of hardware presence. Reconnecting the same endpoint allows retry. A failed audio preflight does not consume the one-time answer guard. GUI refresh preserves an absent session-bound ID instead of silently replacing it. Live mid-call audio migration is not implemented.

Device IDs are opaque ASCII, max127 bytes to fit pinned Baresip's128-byte buffers. Config separators, quotes, escapes/control characters and reserved case-insensitive "default" are rejected. IDs are never logged or persisted. This limitation may reject nonstandard endpoint IDs rather than truncating or misrouting them.

Automated gates: portable route/presence/admission tests; real SDK parser assertions for exact source/player IDs and default mapping; native route snapshot and GUI fixed-selection smoke; full default Windows suite; six real local Digest/TLS scenarios; candidate installer install/file-integrity/uninstall smoke. These gates are required on the final PR head.

The tested0.51 baseline installer remains linked in NATIVE-RELEASE-CANDIDATE.md; the0.52 PR provides its own exact-head artifact. Default SDK-free GUI behavior stays intact. Real Digitro calls, RTP audibility, headset/hotplug hardware acceptance, SRTP and production signing remain unapproved and cannot be inferred from configuration-parser tests.
