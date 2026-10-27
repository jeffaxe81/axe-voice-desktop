# Security feedback implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Candidate0.63 delivers privacy-safe destination refusals and closes thread-exception/manifest-type gaps.
**Architecture:** Pure enum/result/message functions preserve routing wrappers; engine records feedback under mutex. Existing startup cleanup catches failed allocation. Package gate validates scalar schema before comparisons.
**Tech Stack:** C++17/Win32, pinned SDK, CMake, PowerShell.
**Spec:** docs/superpowers/specs/2026-10-27-security-feedback-design.md

## Global Constraints
- Keep final URI limit512 and current transport policy.
- No raw destination/account in error messages or exports.
- Thread exception cleanup joins, erases pending password and publishes Failed.
- Manifest fields use exact scalar types before value/hash comparisons.

## Review Focus
- Expanded destination512/513 and Unicode must retain reason specificity.
- Valid attempts/stop must clear stale refusal feedback.
- Allocation failure during thread creation must not escape or retain password.
- TLS arrays/strings and malformed hash types cannot satisfy provenance gates.
- F2/clipboard stay boolean-only across all2048 combinations.

### Task1: Red tests and minimal fixes
- [x] Four independent testers deliver result API tests, thread-allocation mode5, malformedmanifest regression and export isolation matrix.
- [x] Root independently reproduces RED; implement result API, safe startup and strict types.
- [x] Verify new/affected Release suites and actualSDK five-mode probe.

### Task2: Integration
- [x] Wire refusal feedback to engine/UI and reset boundaries; test actualengine withoutnetwork.
- [x] Add CMake tests, native mode5 and PowerShell CI gate; metadata0.63.0 and documentation.
- [x] Cross-build/native UI/Python; independent whole-branch review.
- [ ] PR exact-head Windows/native/installer gates, merge and verify main.
