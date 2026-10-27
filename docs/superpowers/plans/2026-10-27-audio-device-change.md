# Audio device change plan

**Goal:** Candidate0.60 adds recorded notification/debounce/fallback functionality.
**Spec:** docs/superpowers/specs/2026-10-27-audio-device-change-design.md

- [x] Front1: terminal lock-free coalescing signal and concurrency tests.
- [x] Front2:250ms debounce,5000ms fallback, dropdown/navigation policy and WM event whitelist.
- [x] Front3: optional IMMNotificationClient observer, COM ownership and fresh signal on restart.
- [x] Front4: GUI polling before inventory, generation invalidation, fixed routes, silent smoke.
- [x] Front5: offline help for automatic inventory and unchanged audio limitations.
- [x] Front6: fake COM tests for HRESULT failures, QI/refcounts, callbacks and teardown.
- [x] Front7: signal/policy/worker integration with blocked probes and stale-result exclusion.
- [x] Front8: independent public COM contract audit.
- [x] Front9: remaining-functionality traceability matrix.
- [ ] Front10/root: final review, CMake/version, full Windows/native/installer gates and exact-head merge.

Review fixes: prevent fallback starvation for probes exceeding5s; pace from completion without losing dirty events; use always-on checks in Release tests instead of assert.
