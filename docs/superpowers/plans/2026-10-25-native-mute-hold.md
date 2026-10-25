# Native mute/hold implementation plan

**Goal:** Candidate0.53 offers native mute and local hold/resume with actual SDK readback and call identity fences.
**Spec:** docs/superpowers/specs/2026-10-25-native-mute-hold-design.md

- [ ] Write failing session/action/media-snapshot and SDK-port tests; implement bounded desired-state controls.
- [ ] Integrate owner-thread media snapshot and control dispatch; preserve registration and report errors precisely. Resume uses audio-route preflight.
- [ ] Wire native GUI buttons/readback and update candidate0.53 help/checklist/installer metadata.
- [ ] Run full Windows/native/installer CI; independent review, fix findings and merge exact tested head.
- [ ] Deliver updated candidate; keep physical mute and actual PBX hold acceptance pending.
