# Copy local diagnostic implementation plan

**Goal:** Candidate0.56 provides a private local status copy for support while hardware acceptance stays pending.
**Spec:** docs/superpowers/specs/2026-10-25-copy-diagnostic-design.md
**Architecture:** A pure boolean-only CRLF formatter feeds a focused Win32 clipboard wrapper; existing main readiness collection supplies the same snapshot to F2 and copy. No new SIP logic.

## Task1 — format and clipboard service
- [ ] RED: tests/test_diagnostic_export.cpp expects format_diagnostic_export(CallReadiness) with CRLF, candidate version and independent dial/media indicators.
- [ ] RED: tests/test_diagnostic_clipboard.cpp with tests/clipboard_stub/windows.h compiles src/diagnostic_clipboard_win32.cpp; simulate allocation/lock/open/empty/set failure and successful ownership transfer. No real clipboard writes.
- [ ] GREEN: src/diagnostic_export.h formats booleans only; src/diagnostic_clipboard_win32.h/.cpp allocates movable memory, fills/terminates, transfers CF_UNICODETEXT, closes/frees on correct paths. Reject null owner before allocation.
## Task2 — interface and release
- [ ] Collect CallReadiness in existing main helper; retain F2 wrapper and add COPIAR STATUS at Audio y690 beside F2, plus Ctrl+Shift+D. Copy handler uses service only outside smoke, reports success/failure. Default/native smoke exercise button and accelerator-command paths with no clipboard side effect.
- [ ] Update ui_help, package0.56 metadata, roadmap/checklist and docs/integration/NATIVE-DIAGNOSTIC-COPY.md; explicitly leave PBX/headset and physical clipboard acceptance pending.
- [ ] Run portable tests and Windows crossbuild; independent review; full Windows and native SDK/installer CI; merge exact approved head and verify merged tree equals tested tree.

## Review focus
- Password/server/destination/device values never reach clipboard formatter.
- Every failure before ownership transfer frees memory; every open closes.
- Report states describe UI availability without certifying audio.
- Default/native smoke never call the real clipboard API.
- Button fits existing viewport and keyboard command works from all pages.
