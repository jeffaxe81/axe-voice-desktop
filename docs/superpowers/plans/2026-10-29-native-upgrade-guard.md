# Native upgrade guard Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans.

**Goal:** Reject cooperating app/setup overlap without terminating calls or imposing single-instance behavior.
**Architecture:** Reciprocal process-token-SID Global markers; app RAII holder outlives runtimes, Inno script owns setup reservation through mutation and releases before postinstall launch. Existing-EXE exclusive-write probe supplies conservative legacy protection.
**Tech Stack:** C++17/Win32, InnoSetup6 32-bit script, PowerShell/CTest.
**Spec:** docs/superpowers/specs/2026-10-29-native-upgrade-guard-design.md

## Global Constraints
- Candidate0.68.0; Global\AXEVoiceNativeCandidate.{Running|Setup}.<TokenUserSID>.
- Marker existence only, no owned mutex or forced shutdown. Accept multiple app holders.
- Only ERROR_FILE_NOT_FOUND means absence. Busy GUI exit170; protection error5.
- Protect native candidate only; do not promise historical guarded startup, atomic rollback or obsolete-file removal.

## Review Focus
- Both simultaneous participants must never be admitted.
- Marker lifetime must include global SIP/audio teardown.
- Postinstall [Run] must launch after reservation release.
- SID marshalling must match current Inno6 32-bit engine and fail closed.
- Rejected update/uninstall must leave payload unchanged and real GUI alive.

### Task1: Behavioral regression then implementation
**Files:** src/native_install_guard.h, src/main.cpp, installer/native-upgrade-guard.iss, installer/axe-native-candidate.iss, scripts/package-native-candidate.ps1, tests/test_native_install_guard.cpp, tests/test_native_upgrade_guard.ps1, CMake/version/docs.
**Interfaces:** NativeInstallGuardNames{wstring application,setup}; optional native_install_guard_names()noexcept; NativeInstallGuard::acquire(names)->Acquired/Busy/Failed noexcept, release(), destructor. PS Invoke-NativeUpgradeGuardChecks takes paths, names and busycode.
- [ ] Register packaging behavioral fixture before production fix and observe Windows RED on unlocked payload with held Running marker.
- [ ] Implement name derivation/marker admission and script reservation/probe/cleanup, preserving postinstall launch.
- [ ] Run real Win32 object/race/leak tests, package held-marker/live-GUI/refusal/retry checks; add legacy write-lock fallback regression.
- [ ] Run fullcrossbuild/nativeGUIcompile/24Python/version/diff and exact-head Windows/SDK/installer gates; obtain fresh review, resolve material findings RED→GREEN, authorized PR/mainmerge.
