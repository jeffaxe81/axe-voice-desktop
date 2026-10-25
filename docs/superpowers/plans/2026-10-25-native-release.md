# Native release candidate implementation plan

> Execute inline using superpowers:executing-plans; request independent review before merge.

**Goal:** Deliver a native SDK GUI candidate and installer with automated evidence and explicit remaining hardware/PBX acceptance.
**Architecture:** One native SDK owner thread with bounded command submission, copied runtime snapshots and the existing callback adapter. SDK remains optional, default build unchanged.
**Tech Stack:** C++17, Win32, Baresip/libre4.12, OpenSSL3.5.7, MSVC2022, Inno Setup.
**Spec:** docs/superpowers/specs/2026-10-25-native-release-design.md

## Global constraints
No private source publication; no password in config/AOR/log; no false registration or media claims; no TLS verification bypass; existing SDK hash gates preserved.

## Review focus
- Invalid account/destination injection: reject before SDK allocation.
- Wrong password / lost registrar: never retain registered UI state.
- Stop/reconfigure with queued dial: revoke commands and callbacks before UA destruction.
- TLS missing CA / wrong host: reject without plaintext fallback.
- Missing hardware: surface audio failure and keep production acceptance pending.

## Tasks
- [ ] Account and destination boundary: write failing behavioral tests, implement strict native config conversion, run portable tests.
- [ ] Native engine: real lifecycle and loopback registration tests first; implement owner-side configuration/authentication, callback snapshots and call command gating; verify real SDK tests.
- [ ] GUI candidate: add password/CA and register/answer/hangup controls; connect snapshot/status path; run default and candidate GUI smoke.
- [ ] TLS and media: exercise trusted/untrusted/hostname handshake locally; record explicit physical media/PBX dependencies.
- [ ] Installer: package matching candidate executable/DLLs/licenses and test silent install/uninstall in Windows CI.
- [ ] Review and verification: run existing suite plus candidate CI, fix findings, merge exact approved head and update release acceptance record.
