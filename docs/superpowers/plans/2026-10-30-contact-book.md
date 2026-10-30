# Contact book Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans.

**Goal:** Ship ten user-facing contact-book capabilities in native0.69.
**Architecture:** Small pure models/query/JSONcodec plus Win32 store/actions/modeless UI; root integrates native main. Ten feature implementers in waves, not test/audit workstreams.
**Tech Stack:** C++17/Win32, no new third-party dependency.
**Spec:** docs/superpowers/specs/2026-10-30-contact-book-design.md

## Global Constraints
- Native0.69.0,500contacts,64UTF16name,128ASCIIdestination,512KiBstrictUTF8JSONversion1.
- No SIP side effects from contacts; fill only. Modeless, normal main timer processing, scoped keyboard.
- Strong failed-edit/import/save preservation; protect other-instance changes with baseline comparison and exclusive lock.

## Review Focus
- Hostile/malformed/oversized backup cannot discard existing data or allocate unbounded memory.
- Save conflict/failure leaves existing file and live model unchanged.
- Stale selection IDs cannot select another item after deletion/filter/import.
- Contact keyboard actions cannot invoke main call shortcuts.
- Explicit clipboard/export only, no credentials/diagnostics leakage.

### Task1: Implement model/query/favorites/codec/store and modelessUI
- [ ] Build against exact headers; root verifies minimal contracts and existing suite.
### Task2: Implement import/export/clipboard/dialerbridge
- [ ] All feature actions wired to real UI, persistence and messages.
### Task3: Integrate and deliver
- [ ] Main/CMake/version/help/docs, native GUI smoke without disk mutation; independentreview; final exactheadWindows/SDK/installer gates and authorizedPR/mainmerge.
