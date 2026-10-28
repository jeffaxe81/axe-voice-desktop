# Candidate0.65.1 — partial process startup ownership

## Problem and delivery

The SDK-free foundation can launch a fixed auxiliary executable through WindowsProcess. It creates a job, stdin pipe and NUL output before preparing attribute storage and command buffers. Allocation exceptions previously skipped the manual cleanup of local handles. This is resource exhaustion/failure recovery; no credential exposure or command execution bypass has been demonstrated.

The proposed correction gives each local handle and initialized process attribute list scoped ownership, then maps allocation failure to the existing bool/error/state API. The child remains suspended until assigned to the kill-on-close job; executable selection, argument quoting, inherited-handle allowlist and no-shell launch remain enforced.

## Evidence

- Windows allocation-failure sweep: RED/GREEN evidence is recorded in the associated PR and Actions runs. The test measures actual process handle counts and checks recovery.
- Four concurrent specialist audits: process ownership found the issue; GUI/clipboard privacy, native recovery and package integrity found no additional concrete defect in reviewed scenarios.
- GUI/privacy: diagnostic export, clipboard failure/ownership, 2,048 report combinations/192 proof observations and credential isolation passed on host C++.
- Reactor: actual libre partial cleanup, first-fault preservation and 300 repeated owner generations with concurrent wake passed. Concurrent start/stop is outside the documented serialized lifecycle.
- Packaging: six isolated PowerShell security suites and three CMake integrity suites passed (9 Baresip, 6 libre, 8 provenance scenarios).

## Limits

Host tests and cross-compilation do not execute Windows GUI or physical hardware. Final Windows/SDK/installer gates must correspond to the final PR head. Per-user candidate installer is unsigned. Reinstalling the same candidate does not establish all historical upgrades. Dígitro/headset acceptance and SRTP remain pending; SIP TLS does not encrypt RTP.
