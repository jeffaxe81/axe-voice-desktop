# Security status implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Candidate0.62 adds truthful session policy report and closes reproduced startup/path/host gaps.
**Architecture:** Pure boolean formatter; engine-owned mutex metadata; existing F2/clipboard render it. Four independent tests precede root implementation.
**Tech Stack:** C++17/Win32, CMake, actual pinned SDK, PowerShell.
**Spec:** docs/superpowers/specs/2026-10-27-security-status-design.md

## Global Constraints
- No secrets or identifiers in export; no SRTP/TLS negotiation or production acceptance claim.
- Host253 and label63 maximum, ASCII alphanumeric edges; no live DNS/PBX.
- Startup failure joins before wiping pending password and publishing Failed.
- SDK redirect/reparse paths rejected before staging.

## Review Focus
- Editing TLS checkbox must not change configured engine metadata.
- Stopped/failed states never show current registration.
- Queue failure and init failure skip hooks yet erase pending password.
- Trailing dot before port and empty hostname must be rejected.
- Directory junction payload must not bypass SDK hash enumeration.

### Task1: Red regressions and minimal fixes
- [x] Four specialists create distinct tests for status, hosts, startup failure and linked SDK directory.
- [x] Root reproduces failures then implements formatter/metadata, shared host validator, false-start cleanup and SDK path guard.
- [x] Verify pure and actual-SDK regressions with Release checks active.

### Task2: Integration
- [x] Attach report to F2 and clipboard; add native probe coverage for metadata and portable startup failure modes.
- [x] Register CMake tests, native profile probes and PowerShell test; version0.62.0/help/checklist/roadmap.
- [x] Full Windows cross-build, native UI compile, Python and affected suites; independent whole-branch review.
- [ ] Exact-head PR gates Windows/native/installer, then merge and verify main tree.
