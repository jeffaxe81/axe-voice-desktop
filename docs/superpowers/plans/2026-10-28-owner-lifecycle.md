# Owner lifecycle implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Candidate0.65 cleans partial owner startup, preserves first fault and rejects stale configuration intent.
**Architecture:** Existing reactor cleanup path runs stopping hook after attempted start; first-fault latching and fixed diagnostic. Inactive validation reuses fail-closed credential cleanup; manifest validates canonical names before hashing.
**Tech Stack:** C++17, Win32, pinned libre/Baresip, PowerShell/CMake.
**Spec:** docs/superpowers/specs/2026-10-28-owner-lifecycle-design.md

## Global Constraints
- Cleanup exactly once on owner after start attempt; none before libre/fila succeeds.
- First non-None reactor fault wins; explicit stop clears visible failure.
- Rejected inactive configuration discards prior pending identity; active session untouched.
- Manifest names canonical relative paths; no raw identifiers in diagnostics.

## Review Focus
- Hook throws after acquiring partial state; retry cannot retain owner slot.
- Tick failure followed by loop/cleanup failure keeps original cause.
- End must distinguish spontaneous fault from explicit UI stop.
- Invalid B cannot arm register of previous A across all validation branches.
- Windows path aliases/ADS/traversal rejected before filesystem reads.

### Task1
- [x] Four specialists deliver RED cleanup, first-fault, pending-config and manifest-name regressions.
- [x] Root reproduce failures, implement minimal changes and integrated owner fault probe.
- [x] Release tests and actualSDK probes GREEN, no new SIP traffic.

### Task2
- [x] CMake/CI/version0.65.0/help/docs; preserve export privacy.
- [x] Whole-branch independent review and crossbuild/native UI/Python/PowerShell gates.
- [ ] Exact-head Windows/native/installer PR validation, merge and verify main.
