# Windows process resource recovery Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans. Steps use checkbox syntax.

**Goal:** Release partial Win32 launch resources on allocation failure and permit retry.
**Architecture:** Scoped HANDLE and initialized attribute-list ownership inside WindowsProcess::start; allocation exception maps to existing boolean/error contract.
**Tech Stack:** C++17, Win32, MSVC CTest.
**Spec:** docs/superpowers/specs/2026-10-28-process-resources-design.md

## Global Constraints
- Fixed executable, inherited-handle allowlist and job-before-resume unchanged.
- Candidate0.65.1; hardware, signing and SRTP pending.

## Review Focus
- A failed allocation before any Win32 acquisition leaves no stale Running state.
- Failure after attribute-list initialization destroys the list before backing storage.
- A successful launch transfers pipe/process ownership exactly once.
- Later attempts work after a rejected launch.
- Existing normal launch, quit and forced termination still pass.

### Task 1: Resource-safe process startup
**Files:** src/windows_process.cpp, tests/test_security_process_resources.cpp, CMakeLists.txt, release metadata and integration documentation.
**Interfaces:** WindowsProcess::start(path,path) retains bool return; failure reports ERROR_NOT_ENOUGH_MEMORY and Failed.
- [ ] Register an allocation sweep against actual Windows handles; observe RED on baseline Windows CI.
- [ ] Add exception-safe local resource ownership and bad_alloc boundary; strengthen test to require false/error/Failed and retry after each fault.
- [ ] Run whole Windows suite plus SDK/installer gates on final SHA, version check, Python suite and cross-build.
- [ ] Obtain fresh whole-branch review; resolve material findings with RED→GREEN.
- [ ] Commit, create PR and merge exact approved/passing head under existing authorization.
