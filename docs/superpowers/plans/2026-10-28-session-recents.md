# Session recent destinations Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans.

**Goal:** List, reuse and clear up to10 local accepted dial destinations in the current verified session.
**Architecture:** GUI-owned bounded RecentDestinations model with session/token validation, native popup menu and existing manual CHAMAR authorization. No persistence or exported history.
**Tech Stack:** C++17, Win32, CMake/CTest.
**Spec:** docs/superpowers/specs/2026-10-28-session-recents-design.md

## Global Constraints
- Version0.66.0,128-character original typed input,capacity10, exact deduplication.
- Valid Running/Verified/sessionnonzero context with Registered/Calling/Ringing/InCall; record only idle Registered and accepted engine command.
- No automatic call, disk storage, outcome/incoming metadata or diagnostic payload changes.

## Review Focus
- A popup may outlive proof/session; revalidate tokens after closing it.
- Reloading an unprefixed URI must preserve original text and avoid truncation.
- Allocation failure after accepted dial must not escape or report the call rejected.
- Clearing and eviction revoke stale selections without reusing identifiers.
- Compact/DPI/scroll and native Windows focus/keyboard must remain usable.

### Task1: Model and GUI integration
**Files:** src/recent_destinations.h, src/recent_destination_ui.h, src/main.cpp, src/ui_help.h, four tests/test_recent_destinations*.cpp, tests/test_recent_destination_win32.cpp, CMakeLists.txt, release metadata/docs.
**Interfaces:** RecentDestination{uint64_t token,wstring destination}; RecentDestinations::synchronize(obs),remember(obs,destination,accepted)boolnoexcept,size(),at(index),find(token,obs),clear(). GUI lookup only fills field; explicit manualCHAMAR unchanged.
- [ ] Observe RED for new model tests; implement bounded MRU with nonthrowing transactional updates and context invalidation.
- [ ] Wire native menu, Ctrl+H, selection/clear and accepted-only recording; add native GUI smoke for empty state/geometry and shared Win32 helper coverage for fill/stale selection plus model clear.
- [ ] Run four host tests,24Python/version/diff/full Windows crossbuild/nativeGUI compile, then final-head Windows/SDK/installer CI.
- [ ] Obtain fresh final review, fix material findings with RED→GREEN and suite, create PR and merge passing reviewed head under existing authorization.
