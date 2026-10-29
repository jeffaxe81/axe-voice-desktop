# Eight-front hardening Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans.

**Goal:** Fix complete registration validation and ringtone worker failure containment, finish installer minor followups.
**Architecture:** Seven disjoint audit/implementation/test/documentation specialists followed by eighth fresh wholebranch reviewer; root integrates/tests/merges. Shared native contracts unchanged.
**Tech Stack:** C++17, Win32, PowerShell, Inno6.
**Spec:** docs/superpowers/specs/2026-10-29-eight-front-hardening-design.md

## Global Constraints
- Candidate0.68.2. Publicworktree only, no privatehistoricalsource. No processkill/testhooks/fakeInno/sourcepattern tests.

## Review Focus
- Invalid suffix beyond15characters cannot reach engine or consume password; valid128-character zero-padded port preserved.
- Worker copy allocation failure cannot terminate app or retry hot; stop/generation/shutdown remain live.
- Unexpected affirmative installer dialogs must not be accepted.
- Secondary cleanup failure never replaces primary acceptance failure; cleanup-only remains failure.
- Actual Inno wrong-type/access-denied marker rejects without payload change, releases reservation and retries.

### Task1: Registration and audio fixes
- [ ] Persist RED repros, implement minimal fixes, run focused pure/Win32 tests and whole suite.
### Task2: Installer errors and cancellation
- [ ] Pure cancellation/exception REDGREEN contracts, real Inno error injections, wire packaging and exacthead nativeCI.
### Task3: Eighth review and release
- [ ] Correct0.68.1 evidence docs, versionmetadata, fresh most-capable review, resolve material findings REDGREEN, final exacthead gates and authorizedPR/mainmerge.
