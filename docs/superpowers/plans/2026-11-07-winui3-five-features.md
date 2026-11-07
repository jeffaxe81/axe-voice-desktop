# Five Fronts — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Five practical WinUI 3 preview features, no change to Win32 SIP owner.

**Architecture:** Extend the C++ read-only presentation bridge with validated destination checks. Implement pure C# feature models to isolate user preferences, navigation and diagnostics; wire into compact WinUI XAML. Use CTest for C++ and a self-contained .NET console contract test for C#.

**Stack:** C++17, CMake/CTest, .NET 10 WinUI 3 Windows App SDK 2.6.5, PowerShell, GitHub Actions.

### Front 1 — SIP destination validation (TDD)
- [ ] Write bridge tests for valid extension/SIP URI, invalid/empty input and bounded length.
- [ ] Observe failing CI when the API is absent.
- [ ] Implement `axv_frontend_validate_destination` in C ABI with `axv_sip_address` library.
- [ ] Connect WinUI 3 display validation; offline mode still prohibits Dial.

### Front 2 — Physical keyboard (TDD)
- [ ] Write pure `DialerTextModel` regression tests including backspace/escape and max length.
- [ ] Implement pure model; wire physical shortcuts into input, without SIP side effects.

### Front 3 — Local encrypted favorites (TDD)
- [ ] Test add/remove/duplicate/max 10, plus corrupt file and encrypted persistence.
- [ ] Implement DPAPI CurrentUser protected local storage (explicit save only).
- [ ] Show add/load/delete action in WinUI 3 preview. Never call saved destination.

### Front 4 — Secure SIP diagnostics (TDD)
- [ ] Test status report excludes username/server/number/secret and offline state not green.
- [ ] Implement refresh and copy using read-only C ABI snapshot, with no SIP commands.
- [ ] Add displayed diagnostic text and accessible actions to Monitor tab.

### Front 5 — Size modes (TDD)
- [ ] Test Compacto=355x602, Conforto=400x680, invalid choice falls back safely.
- [ ] Implement mode switch and accessible settings control.
- [ ] Check geometry remains scroll-free and does not alter the current Win32 UI.

### Integration
- [ ] Run C++ CTest + C# model checks in GitHub Windows runners.
- [ ] Run actual WinUI 3 build + theme quality gate.
- [ ] Review native SDK smoke and Win32 CI; merge only if all green.
- [ ] Publish separate WinUI 3 artifact with explicit **No-SIP** label.
