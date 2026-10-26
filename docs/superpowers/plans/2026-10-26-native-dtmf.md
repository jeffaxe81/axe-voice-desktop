# Native DTMF implementation plan

**Goal:** Candidate0.57 adds explicitly armed SIP INFO digits to established native calls.
**Spec:** docs/superpowers/specs/2026-10-26-native-dtmf-design.md
**Architecture:** Shared policy, call-bound native queue, SDK port, and native-only keypad mode. No RTP tone scheduler.

## Task1 — guarded native command
- [ ] RED tests/test_native_dtmf.cpp: key whitelist, identity/proof/hold/transfer gating, command payload and repeat admission. Implement src/native_dtmf.h and Dtmf action in native_session_control.h.
- [ ] RED extend tests/test_baresip_sdk_call_port.cpp and fake SDK declarations for call_send_digit, call_account and account_dtmfmode. Implement send_dtmf_info(token,key), guarded for established/owner/hold/transfer/mode, preserving call on failure and reentry.
- [ ] Add dtmfmode=info to account profile with portable assertion and real account parser assertion. Wire BaresipSipEngine::send_dtmf(wchar_t,const NativeCallMediaState&) and owner dispatch, fixed last-error feedback reset on identity changes. Real SDK linkage follows native build.
## Task2 — interface and release
- [ ] Add native DTMF toggle below the call controls; native full viewport gains44px and keeps scrolling. Arm displayed identity, use on-screen digits only in mode, reject stale press, clear on invalid state. Update silent native UI smoke and no-call engine test.
- [ ] Version0.57.0, help, candidate README, roadmap/checklist and integration limits. Physical PBX/headset/INFO acceptance stays pending.
- [ ] Portable policy/SDK tests and crossbuild/native compile; independent review; final Windows/native SDK/installer CI; merge exact approved head and verify tree.

## Review focus
- Stale displayed identity never sends to a replacement call.
- Duplicate digits can be intentional; queue overload is visible.
- Non-INFO account never starts an unbounded RTP tone.
- No native pointer use after reentrant send; error never tears down call.
- DTMF is explicitly armed and does not consume destination edits when off.
