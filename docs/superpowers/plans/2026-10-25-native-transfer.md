# Native direct transfer implementation plan

**Goal:** Candidate0.54 clears stale call feedback and offers a protected direct transfer request.
**Spec:** docs/superpowers/specs/2026-10-25-native-transfer-design.md

- [ ] Regression test feedback cleanup on close/replacement; implement shared production helper and preserve same-call errors.
- [ ] Write URI/TLS/admission/SDK-port tests; implement selected-call direct transfer and failure readback.
- [ ] Wire expected displayed identity through native engine and destination-review UI; add ABI/no-call/GUI checks.
- [ ] Run full Windows/native/installer checks, independent review and corrections; merge exact tested head and provide candidate installer.
- [ ] Leave actual central transfer/NOTIFY/media acceptance explicit and pending.
