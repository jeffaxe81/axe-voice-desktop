# Parallel release hardening plan

**Goal:** Deliver candidate0.57.1 using five independent developer fronts.
**Spec:** docs/superpowers/specs/2026-10-26-parallel-hardening-design.md

- [x] Calls/DTMF audit and focused sanitizer tests; no concrete defects, no changes.
- [x] Audio: reproduce failed worker creation and repair retry state; focused regression passes.
- [x] UI: add BS_NOTIFY for buttons/TLS and real-control focus smoke; portable contract RED->GREEN.
- [x] Diagnostics audit and sanitizer tests; no concrete defects, no changes.
- [x] Packaging: bind manifest to tested binary/version, validate dependencies, rebuild stage; PowerShell negative fixtures pass locally.
- [ ] Integrate disjoint changes, run Windows/native/installer gates and independent whole-branch review; merge exact validated head.

Review focus: worker retry does not create duplicate threads; focus notifications do not trigger calls; smoke remains silent; hash checks reject invalid inputs before staging; install/upgrade/uninstall gates remain real and provenance is not advertised as a signature.
