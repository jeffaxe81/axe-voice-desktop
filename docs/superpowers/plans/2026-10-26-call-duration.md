# Native call duration plan

**Goal:** Deliver candidate0.58 with local duration and stronger release/fixture checks.
**Spec:** docs/superpowers/specs/2026-10-26-call-duration-design.md

- [x] Front1: NativeCallDuration pure clock with proof/identity/reset/backward/overflow RED->GREEN tests.
- [x] Front2: format_call_duration HH:MM:SS, long hours and uint64 formatting tests RED->GREEN.
- [x] Front3: native UI label ID2503 at42,190,347,20, shared observation/GetTickCount64; inactive and MINI/layout smoke; help.
- [x] Front4: Python release metadata consistency gate and isolated fixture tests before Windows build.
- [x] Front5: real TLS delayed-handshake regression; separate handshake/read timeouts while preserving all TLS assertions.
- [ ] Root: integrate CMake/version/docs, run combined tests, independent review and final Windows/native/installer gates; merge exact validated head.

Review focus: unverified periods never count; elapsed time is local rather than network/media evidence; MINI/DPI/scroll label visibility; release markers cannot silently diverge; TLS delay tolerance preserves strict certificate and hostname rejection.
