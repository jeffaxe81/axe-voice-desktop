# Security hardening implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Deliver candidate0.61 with fail-closed transport intent, input and package integrity protections.
**Architecture:** Keep shared address validation, native routing and real packaging gates. Four testing agents own disjoint new regression files; root implements production fixes and integrates.
**Tech Stack:** C++17, Win32, CMake, PowerShell, GitHub Actions.
**Spec:** docs/superpowers/specs/2026-10-27-security-hardening-design.md

## Global Constraints
- Preserve explicit SIP/SIPS; no silent downgrade.
- Native final URI maximum512 bytes, supported ASCII only; no percent escapes.
- Username alphabet ASCII letters/digits and _ . - +.
- No actual credentials, private sources, target PBX calls or TLS verification bypass.

## Review Focus
- SIP/SIPS short forms must survive authorization normalization and avoid duplicated schemes.
- Generated host/port/transport must count toward the512-byte limit.
- Invalid user metacharacters fail both form and native validation.
- Literal bracket filenames cannot alias another file during hashing.
- Stale identity and callbacks remain rejected after restart; diagnostics remain boolean-only.

### Task1: SIP/account boundary
- [x] Four independent specialists create regression tests; root reproduces RED for SIP and credential validation.
- [ ] Preserve explicit short-form scheme in src/sip_address.cpp; enforce matching account transport, URI byte limit and native user whitelist in src/native_account_config.h; align src/sip_account.h.
- [ ] Run new security C++ tests plus affected existing tests with -DNDEBUG.

- [x] Reproduzir retenção de senha pendente com SDK real; apagar antes de publicar falha e executar probe nativa sem rede.

### Task2: Literal package integrity
- [x] Reproduce actual package gate accepting incorrect hash for bracket filename in tests/test_security_package.ps1.
- [ ] Replace path-based Get-FileHash calls with -LiteralPath in scripts/package-native-candidate.ps1.
- [ ] Verify isolated red/green regression; wire it before actual SDK package gates.

### Task3: Integration
- [ ] Register three C++ tests with CMake and quality labels; add PowerShell security regression to native CI.
- [ ] Set all release metadata to0.61.0, update help/security ledger/roadmap and preserve external pending items.
- [ ] Full cross-build, Python suite, native UI compilation, independent final review.
- [ ] PR with exact-head Windows/native/install gates; merge only on success and verify resulting main tree.
