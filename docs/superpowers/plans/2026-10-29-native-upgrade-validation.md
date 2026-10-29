# Native upgrade validation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans.

**Goal:** Validate production installer handoff/contention and keep app protection through actual process exit.
**Architecture:** Six independent audits/test/docs fronts; root integrates. Explicit process lifetime guard complements existing scoped RAII. Child process composes real guard with detached inventory worker.
**Tech Stack:** C++17 Win32, PowerShell, Inno6/WindowsCI.
**Spec:** docs/superpowers/specs/2026-10-29-native-upgrade-validation-design.md

## Global Constraints
-0.68.1, native only; unchanged SID marker names/admission.
-No process killing, owned-mutex protocol, fake interactive installer or production test hooks.

## Review Focus
-Blocked detached probe survives owners while process marker remains; OS releases after child exit.
-Production [Run] launches checked app after setup reservation released.
-Actual overlapping setup/uninstall reject without payload mutation; cancellation releases marker.
-Bounded GUI automation scopes actions to started process identities; unexpected dialog fails clearly.
-Real TokenUser-derived names and malformed/reacquire guards checked independently.

### Task1: Process lifetime
- [ ] Add child-process composition regression using real guard+AudioInventoryAsync, observe existing scoped behavior fail lifetime requirement.
- [ ] Add explicit process lifetime constructor option, use in GUI; run regression/fullWindows.
### Task2: Production installer validation
- [ ] Integrate new interactive/contention fixtures and independent SID checks.
- [ ] Run final exact-head Windows/SDK/Inno behavioral gates and fresh independent review.
### Task3: Release
- [ ] Update version/docs with evidence, PR and authorized main merge after passing final checks.
