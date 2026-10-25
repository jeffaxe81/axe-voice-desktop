# Incoming ringtone implementation plan

**Goal:** Candidate0.55 alerts verified incoming calls on an independent selected output and cancels obsolete sound safely.
**Spec:** docs/superpowers/specs/2026-10-25-incoming-ringtone-design.md

- [x] Write failing ring/provenance/suppression/default-route policy tests; implement call-bound request policy.
- [x] Write deterministic single-worker cancellation/replacement/error/shutdown tests; implement render worker and cancellable WASAPI service.
- [x] Wire native UI incoming alert, preview exclusion and displayed-call answer/hangup identity; update0.55 docs/package metadata.
- [ ] Run full Windows/native/installer gates and independent review; correct findings and merge exact verified head.
- [ ] Deliver candidate with actual ringing/audibility/hotplug acceptance remaining external.
