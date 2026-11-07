# Voice Application Service Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extract the Win32 SIP call-command and observation boundary into reusable, testable native C++ service without modifying the engine or enabling unverified WinUI 3 calls.

**Architecture:** A C++17 template facade delegates to a BaresipSdk-compatible port, returns an immutable presentation snapshot and double-checks displayed/current SIP identity before forwarding commands. Win32 remains owner of the concrete engine; the service is a non-owning adapter in this stage. WinUI 3 remains fail-closed and gains a reusable native service to integrate in the subsequent phase.

**Tech Stack:** C++17, Baresip existing, Win32/MSVC, CTest, GitHub Actions Windows/native SDK/WinUI 3.

**Spec:** `docs/superpowers/specs/2026-11-07-voice-application-service.md`

## Global Constraints
- Reuse the exact native session and call-token guard; no fabricated registration.
- Preserve the Win32 Essencial interface and existing SIP SDK owner.
- No secrets in service, logs, C ABI or test files.
- No SIP traffic or account changes from unit tests.
- If a command arrives after a transition, return false and do not invoke native port.

## Review Focus
- Delayed incoming ANSWER after new SIP session must be rejected.
- HANGUP must not terminate a different displayed call.
- Dialing on Calling/Ringing/InCall must be rejected even if destination valid.
- Native command queue rejection must propagate failure without claiming connected.
- WinUI 3 without a native engine remains offline with Dial disabled.

---

### Task 1: Define behavior with regression tests

**Files:** `tests/test_voice_application_service.cpp`, `CMakeLists.txt`.

**Interfaces:** `VoiceApplicationService<Port>::snapshot(bool dial_authorized)`,
`dial(displayed, destination)`, `answer(displayed)`, `hangup(displayed)`.

- [ ] Step 1: Write failing C++ regression tests covering current/stale/invalid
  states and a fake port whose SDK-call counters are inspected.
- [ ] Step 2: Verify the CTest Windows build fails as expected because the header is absent.
- [ ] Step 3: Commit tests.

### Task 2: Implement shared service and integrate Win32

**Files:** `src/voice_application_service.h`, `src/main.cpp`.

- [ ] Step 1: Implement minimal native service for trusted snapshots and guarded
  Dial/Answer/Hangup with delegated native command admission.
- [ ] Step 2: Route Win32 native status, dial, answer and hangup through the
  application service; retain legacy UI and non-native build.
- [ ] Step 3: Verify test binary passes and no regressions in Windows/native SDK.
- [ ] Step 4: Commit functionality.

### Task 3: Verify dual-edition regression

**Files:** `docs/architecture/DUAL-FRONTENDS.md`.

- [ ] Step 1: Document real shared C++ command layer vs still-offline WinUI 3.
- [ ] Step 2: Run Windows quality gates, WinUI 3 build, native SDK smoke and
  reactor repeat stability checks in GitHub Actions.
- [ ] Step 3: Review entire branch, merge only when all checks green.
- [ ] Step 4: Confirm no credential or real-call claim was introduced.
