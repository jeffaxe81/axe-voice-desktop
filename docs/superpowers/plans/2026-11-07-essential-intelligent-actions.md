# Essencial inteligente — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Expose verified SIP call actions contextually in AXE Voice's 355×602 compact Win32 view without changing core SIP handlers or the approved appearance.

**Architecture:** A pure C++17 state-to-action projector chooses Idle/Incoming/Outgoing/Conversation from trusted `SipRuntimeObservation`. The existing Win32 control IDs are shown, repositioned and enabled by the validated state, while native command admission remains authoritative. Projection changes, not timer ticks, trigger relayout.

**Tech Stack:** C++17, Win32, Baresip, CMake/CTest, GitHub Actions Windows+native SDK.

**Spec:** `docs/superpowers/specs/2026-11-07-axe-voice-essencial-inteligente-design.md`

## Global Constraints
- Preserve the original Essencial reference and compact frame of **355×602 logical pixels at 96 DPI**.
- No scrollbars, changed SIP credential handling, new codecs or new network/API traffic.
- Reuse existing native IDs `kCallButton`, `kAnswerNative`, `kHangupFuture`, `kMuteFuture`, `kHoldFuture`, `kTransferFuture`.
- Registration proof requires `sip_proof_is_current` and a nonzero call token for visible call actions.
- Tests must not require any external SIP server or customer credentials.

## Review Focus
- Forged Ringing without native proof: UI must not reveal enabled Atender.
- Incoming event with zero call_id: UI must not reveal Atender/Recusar.
- A previous session's call must not become an actionable button.
- Pressing Recusar/Hangup after a call expires must be rejected by the native command handler.
- Resize/DPI 96/120/144 and Monitor→Teclado must never overlap buttons or add scroll.

---

### Task 1: Pure call-action projection (TDD)

**Files:**
- Create: `src/essential_call_actions.h`
- Create: `tests/test_essential_call_actions.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- `enum class EssentialCallView { Idle, Incoming, Outgoing, Connected }`
- `struct EssentialCallActions { EssentialCallView view; bool dial, answer, hangup, secondary; }`
- `EssentialCallActions essential_call_actions(const SipRuntimeObservation&, std::uint64_t active_session, bool dial_authorized) noexcept`

- [ ] **Step 1:** Write independent table-driven C++ tests for stopped, unverified, Registering, Registered/dial authorized, Ringing/call id, Calling/call id, InCall/call id, stale session and zero-token attacks.
- [ ] **Step 2:** Run host `g++ -std=c++17 -Wall -Wextra -pedantic -I. tests/test_essential_call_actions.cpp -o /tmp/axe-essential-test`; expected: FAIL (header missing).
- [ ] **Step 3:** Implement the minimal pure projector, keep `Idle` for any invalid/mismatched proof; wire CTest target.
- [ ] **Step 4:** Run the same host test and verify pass, then confirm Windows CI.
- [ ] **Step 5:** Commit the unit-tested projector.

### Task 2: Contextual Win32 layout and update lifecycle

**Files:**
- Modify: `src/main.cpp`

**Interfaces:** Consume `essential_call_actions` from Task 1; existing BN_CLICKED handlers handle the commands.

- [ ] **Step 1:** Add a state-matrix regression to the existing smoke mode, asserting visible primary and secondary controls and rectangle containment for all 4 states.
- [ ] **Step 2:** Run Windows CI to witness the smoke regression fail before implementation.
- [ ] **Step 3:** In normal Essencial view reuse the original call row for Ligar (idle), Atender/Recusar (incoming), Encerrar (outgoing/connected); secondary row only in connected mode; keep Monitor and preferences intact.
- [ ] **Step 4:** Recalculate layout only on `EssentialCallView` transitions, keep focus on valid visible controls and use current native session authorization when clicking.
- [ ] **Step 5:** Verify new smoke, legacy smoke, existing blackbox and native SDK CI; commit.

### Task 3: Regression review and packaging

**Files:**
- Modify: `docs/superpowers/specs/2026-11-07-axe-voice-essencial-inteligente-design.md` (status and acceptance evidence only)
- Optional: `docs/...` release notes, if needed.

- [ ] **Step 1:** Validate 96/120/144 DPI controls, high contrast and keyboard actions, including Monitor→Teclado.
- [ ] **Step 2:** Inspect whole diff for dangerous command routing, changing `g_displayed_sip`, or misleading SIP status.
- [ ] **Step 3:** Open a PR to main and verify baseline, Windows CI and native smoke checks.
- [ ] **Step 4:** Merge only if all required checks succeed and review has no Important or Critical findings; retrieve installer artifact if available.

**Note:** the hosting environment exposes GitHub connector and host C++ compiler but no Windows runner locally; Windows interactions must be checked by GitHub Actions. Failure or pending results are disclosed, never called passed.
