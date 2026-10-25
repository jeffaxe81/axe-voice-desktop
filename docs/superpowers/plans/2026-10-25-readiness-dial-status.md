# Readiness dial status implementation plan

**Goal:** Correct contradictory F2 call diagnostics in candidate0.55.1.
**Spec:** docs/superpowers/specs/2026-10-25-readiness-dial-status-design.md
**Architecture:** Existing formatter consumes a new default-false dial-control indicator. GUI derives it using authorize_dial with the already read snapshot. Existing six acceptance booleans retain their meanings.

## Task1 — diagnostic correction
- [ ] Add regression in tests/test_call_readiness.cpp: first five gates true, media false must not report CHAMADAS BLOQUEADAS; observe RED.
- [ ] Add dial_control_available to src/call_readiness.h and format actual interface availability separately from checklist summary. Update existing test and silent GUI smoke expectations. Test availability true/false independently of media; all six acceptance gates stay unchanged.
- [ ] Wire main.cpp using authorize_dial(snapshot,snapshot.session,g_sip_selection,number).allowed(); smoke must report unavailable. Patch CMake/installer/title to0.55.1 and document limits.
- [ ] Portable test and full Windows crossbuild; independent review; Windows76 tests and native SDK/installer CI on exact head. Merge approved head to main and verify merged tree.

## Review focus
- Media remains unverified; enabled button never asserts working audio.
- No secrets/typed text in formatter.
- Same presentation authorization for real button and report; engine validates again on action.
- SDK-free smoke remains blocked by real authorization, without simulated proof.
- Existing six-field aggregate users default the new indicator to false.
