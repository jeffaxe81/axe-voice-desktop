# GUI credential recovery Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans.

**Goal:** Contain registration preparation exceptions and erase caller-owned credentials on every exit.
**Architecture:** Extract existing action into one Win32-facing template header used by the GUI and real behavioral tests with WinAPI doubles. Prepare nonsecret owning arguments first; scoped erasure and a result enum contain failures.
**Tech Stack:** C++17, Win32, CMake/CTest, existing native SDK gates.
**Spec:** docs/superpowers/specs/2026-10-28-gui-credential-recovery-design.md

## Global Constraints
- Keep field limits257 wide characters, UTF8 encoding and15000ms registration timeout.
- Keep unauthenticated paths, native engine ownership and call state unchanged.
- Candidate0.65.2; no new logging, credential persistence or hardware approval.

## Review Focus
- Allocation before password read cannot escape WindowProc.
- Configuration argument construction cannot discard an unwiped secret temporary.
- Empty/invalid UTF8 password cannot trigger register.
- Invalid form preserves the password field; exceptional failure clears it.
- Configure/registration refusal or exception leaves no active caller buffer.

### Task1: Registration GUI boundary
**Files:** src/native_registration_ui.h, src/main.cpp, tests/test_native_registration_ui.cpp, tests/test_native_registration_win32.cpp, tests/registration_ui_fakes/windows.h, CMakeLists.txt, release metadata/docs.
**Interfaces:** NativeRegistrationControls holds six HWNDs; NativeRegistrationResult enum InvalidAccount/Refused/Requested/PreparationFailed; register_native_account_ui(engine,controls,inputId,outputId) performs action.
- [ ] Write behavioral fault-injection tests; extract old behavior and observe runtime RED (exception/uncleared buffer).
- [ ] Prepare nonsecret state first and implement scoped erasure/error boundary; test GREEN including invalid forms and encoder/configure/register failures.
- [ ] Wire GUI fixed feedback; test real Win32 controls with valid multibyte UTF8 and unpaired surrogate, then run host tests, full crossbuild,24Python tests, release consistency and actual Windows/SDK/installer CI on finalSHA.
- [ ] Independent final review, resolve material findings with RED→GREEN, then authorized PR/main merge.
