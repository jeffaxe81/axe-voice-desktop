# Security recovery implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Candidate0.64 recovers safely from configuration allocation failure and closes diagnostic/package/argument gaps.
**Architecture:** Scoped input erasure and prepared state commit; existing engine state under mutex. All-file SDK provenance and pure CRT quoting helper feed unchanged process launch.
**Tech Stack:** C++17/Win32, pinned SDK, CMake, PowerShell.
**Spec:** docs/superpowers/specs/2026-10-28-security-recovery-design.md

## Global Constraints
- Never log account/password/paths; no real SIP traffic in new probes.
- Active native sessions remain unchanged on rejected reconfiguration.
- Inactive configure exception erases pending credentials and invalidates configuration.
- Hash every SDK file except its own manifest; include hidden/license files.
- Preserve explicit executable/no-shell/handle restrictions.

## Review Focus
- Allocation failure at early and later prepare/copy stages cannot leave mixed account or secrets.
- Repeated stop clears old SDK messages; retry reports its own failure.
- Added or modified license payload must fail provenance.
- Windows drive roots and trailing separators retain exact argv bytes.
- All Release checks stay active; native module probes fail before account creation.

### Task1: Regressions and fixes
- [x] Four testers provide configure failure, diagnostic lifecycle, license integrity and argument quoting RED probes.
- [x] Root reproduces RED and implements minimal guards/commit, resets, file hashes and quoting.
- [ ] New helpers and SDK probes pass; actual Windows child verifies trailing separator.

### Task2: Integration
- [x] CMake/CI, metadata0.64.0 and docs; no source-only implementation-mirroring tests.
- [x] Cross-build/native UI/Python/PowerShell/SDK probes and independent review.
- [ ] Exact-head Windows/native/installer PR gates, merge and verify main.
