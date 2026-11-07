# AXE Voice Dual Frontends Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship separately buildable Win32 and WinUI 3 app editions, linked to one safe, versioned presentation contract, without duplicating Baresip.

**Architecture:** Keep existing Win32 CMake target and native SIP engine unchanged. A new C++17 shared bridge projects existing native call/session states into an immutable, versioned C ABI. WinUI 3 C#/.NET 10 XAML calls this bridge; calls remain disabled until real native SIP service integration in a later stage.

**Tech Stack:** MSVC/CMake/CTest, C++17, Baresip/libre existing, C ABI, C#/.NET 10, WinUI 3, Windows App SDK stable 2.6.5, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-11-07-dual-frontends-win32-winui3.md`

## Global Constraints

- Keep `AXEVoice` and Win32 release installer unchanged in this stage.
- WinUI 3 is a separate application and does not originate real SIP traffic.
- Never enable a call based on process startup; proof requires verified session and call identity.
- Never expose, store or log SIP secrets via the UI bridge.
- Target Windows x64 initially; Windows App SDK stable 2.6.5 and .NET 10.
- No new scrollbars in compact dialer; preserve brand identity.

## Review Focus

- Missing or incompatible bridge DLL: show offline, never green/connected.
- Invalid ABI struct sizes/versions: reject without reading/writing past boundaries.
- Stale session/call ID: no answered/terminated state as actionable.
- Disabled call button: no queued fake network operations.
- Native Win32 builds must not depend on WinUI package restore.

---

### Task 1: Shared presentation ABI (TDD)

**Files:** `src/frontend_shared/axv_frontend_api.h`,
`src/frontend_shared/axv_frontend_api.cpp`,
`tests/test_frontend_shared_abi.cpp`, `CMakeLists.txt`.

**Interfaces:** `axv_frontend_abi_version()`,
`axv_frontend_get_snapshot(out, size)`, `axv_frontend_project_for_test(...)`.
`axv_frontend_get_snapshot` always reports unavailable until the native
application-service host is linked; project_for_test performs no real I/O.

- [ ] Step 1: Write test cases for ABI version, null/short buffers, offline,
  Registered/Ringing/Calling/InCall with verified/stale sessions.
- [ ] Step 2: Confirm test fails when symbols are absent.
- [ ] Step 3: Implement ABI with `essential_call_actions.h` projection; fail closed.
- [ ] Step 4: Run CTest and MSVC x64 build; check no unexpected dependencies.
- [ ] Step 5: Commit.

### Task 2: WinUI 3 desktop candidate

**Files:** `apps/winui3/AXEVoice.WinUI3.csproj`,
`App.xaml[.cs]`, `MainWindow.xaml[.cs]`, `NativeBridge.cs`,
`apps/winui3/README.md`.

- [ ] Step 1: Write testable acceptance matrix for dialpad, offline state,
  layout and absence of native dialing.
- [ ] Step 2: Create C#/.NET 10 WinUI 3 unpackaged x64 project using the
  supported Windows App SDK stable; do not add a second SIP implementation.
- [ ] Step 3: Create compact screen with keyboard, status, tabs and preferences
  views, disabled real call actions until integration.
- [ ] Step 4: Bind to the native bridge for actual readiness (offline now)
  with DllNotFound/ABI error handling.
- [ ] Step 5: Build on Windows runner; correct compile errors; commit.

### Task 3: CI and release separation

**Files:** `.github/workflows/winui3-preview.yml`,
`docs/architecture/DUAL-FRONTENDS.md`.

- [ ] Step 1: WinUI 3 Windows runner builds frontend bridge (CMake) and
  new C# WinUI 3 project with locked stable Windows App SDK.
- [ ] Step 2: Publish preview artifact under distinct name, not replacing
  `AXE-Voice-Native-Candidate-Setup`.
- [ ] Step 3: Preserve existing Windows CI/Native SDK workflows, check all gates.
- [ ] Step 4: Manual follow-up: test visual and native SIP functionality
  separately on Windows 10/11; future commit implements real SIP bridge.
