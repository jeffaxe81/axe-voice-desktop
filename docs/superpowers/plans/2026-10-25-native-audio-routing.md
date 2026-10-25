# Native audio routing implementation plan

**Goal:** Apply selected capture/playback endpoints to native candidate calls without silent device fallback.
**Architecture:** Validated route value passed at registration; SDK config generation and owner-thread fresh inventory guard before dial/answer. Route snapshot drives locked UI selection and diagnostic feedback.
**Spec:** docs/superpowers/specs/2026-10-25-native-audio-routing-design.md

- [ ] Write failing portable tests for route encoding, missing/default devices, swapped roles, inventory errors and fixed selection without fallback; implement policy and run tests.
- [ ] Write real SDK config parser/snapshot tests; add route to native configuration and guard call actions on owner thread. Keep registration/hangup independent of audio availability.
- [ ] Wire current GUI selections at registration, lock active-session controls, preserve missing selection on refresh, display route errors and update help.
- [ ] Run complete Windows/general/native/installer CI, independent review and fix regression cases.
- [ ] Update recorded release status, merge exact approved head and deliver updated candidate installer. Physical headset/PBX acceptance remains pending externally.
