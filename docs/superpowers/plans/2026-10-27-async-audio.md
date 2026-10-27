# Async audio implementation plan

**Goal:** Candidate0.59 incorporates recorded asynchronous Audio/Settings diagnostics.
**Spec:** docs/superpowers/specs/2026-10-27-async-audio-design.md

- [x] Front1 worker: shared-state lifetime, serial probe, generations and single delivery.
- [x] Front2 retry:1000ms pacing without overflow.
- [x] Front3 selection: errors preserve IDs; native mode never substitutes missing endpoints.
- [x] Front4 UI: two channels, timer-only application, navigation/compact cancellation and silent smoke.
- [x] Front5 help: asynchronous behavior and unchanged route/audio limitations.
- [x] Front6 shutdown tests: return before blocked probe completes, lifetime reclaimed afterwards.
- [x] Front7 epochs tests: concurrent requests coalesce and obsolete results never publish.
- [x] Front8 channel tests: one blocked pane never consumes/blocks the other.
- [x] Front9 failures: thread allocation and probe exceptions recover without stale output.
- [ ] Front10 independent final review; root CMake/version integration and Windows/native/installer gates.

Review focus: no UI/SIP dependencies in probes; cancel is not driver cancellation; failed inventory stays disabled after native status ticks; no fallback changes fixed routes; cross-page results and closed workers never update HWND.
