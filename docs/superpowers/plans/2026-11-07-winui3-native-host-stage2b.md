# WinUI 3 Native Baresip Host Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Enable opt-in native SIP registration and guarded call commands in a separate WinUI 3 laboratory candidate without changing Win32 telephony.

**Architecture:** The same C++ BaresipSipEngine is linked into an SDK-enabled native frontend DLL. No SIP engine is written in C#. A singleton host retrieves an explicitly remembered account from Windows registry/Credential Manager only on user request. A versioned C ABI exposes state and guarded commands; the default Preview DLL remains offline.

**Tech stack:** C++17/Baresip/libre, Win32 Credential Manager, C ABI, C#/.NET 10 WinUI 3, GitHub Actions MSVC/Windows.

**Spec:** `docs/superpowers/specs/2026-11-07-winui3-native-host-stage2b.md`

### Task 1 — ABI and session policy (TDD)
- [ ] Write red tests for v2 ABI and offline functions; reject invalid session and invalid destinations.
- [ ] Implement v2 contract in bridge (offline build fail-closed).
- [ ] Confirm Windows CI/CTest green.

### Task 2 — Native credential loading (TDD)
- [ ] Write tests for missing transport, mismatch identity, invalid port, TLS CA absent and secret erasure.
- [ ] Implement C++ adapter that only reads account from current user's registry and saved secret from Credential Manager on explicit request.
- [ ] Store transport/CA metadata in Win32 **only** with remember-password opt-in.

### Task 3 — Real single-owner host (TDD)
- [ ] Write native SDK smoke test of empty startup, offline status, unauthorized/stale commands and stop; no outbound network.
- [ ] Add optional CMake target linking shared DLL to `axv_native_sip` for native SDK profile.
- [ ] Implement engine owner with `VoiceApplicationService`, locked start/stop, verified snapshot, dial/answer/hangup. Keep no credentials after start.

### Task 4 — WinUI 3 candidate UI
- [ ] Add explicit connect/disconnect actions, live state refresh and contextual buttons, disabled if backend not attached/verified.
- [ ] Recheck C ABI session/call_id on click; never use an Enter shortcut for dialing.
- [ ] Keep the generic WinUI 3 Preview package offline.

### Task 5 — CI/release separation
- [ ] Build/check native host DLL on pinned SDK Windows runner, publish **native candidate ZIP** separate from Preview No-SIP.
- [ ] Run Windows CI, WinUI 3 CI, native SDK and 1,000 loopback tests.
- [ ] Only merge after all required checks pass; label not production approved.
