# Open pull request integration — 2026-10-25

The integration branch preserves main history and merges all seven open PR heads:
12 (bootstrap), 14 (event identities), 15 (native ingress), 23 (pipeline),
31 (callback epochs / registration owner), 36 (viewport), and 39 (regressions).
The product version remains 0.51.0 from the newest bootstrap snapshot. Older
version labels in feature documentation describe their original delivery.

## Conflict decisions

- Keep main's README and preserve the bootstrap README in PR12-BOOTSTRAP-README.md.
- Preserve the v0.51 reducer's monotonic call_id in the third event aggregate
  position. PR14's account_token and call_token are additional explicit fields;
  its identity reducer translates the admitted call_token into the call_id
  reducer. Existing producers and reducer regression tests retain their ABI.
- Preserve both projection contracts: the three-argument session/call projection
  and explicit account-bound projection. Once account-bound mode is selected,
  three-argument observations continue through that mode, so proof revocation
  cannot bypass or revive the selected account generation. snapshot follows the
  active mode. Both original regression suites remain registered.
- Preserve PR15's alternate native ingress and full pipeline tests. Both
  projection paths reject malformed public batch sizes before indexing.
- Combine PR23 account-bound callbacks with v0.51's emitted command tokens,
  single SDK owner, exception-safe callback allocation, and conservative native
  event filtering. Lifecycle captures the selected account identity.
- Combine PR31's registration owner, libre reactor, registration command
  subsystem and all tests with the existing call-command port. SDK callbacks
  hold an outer epoch lease until return. stop waits for these leases before
  releasing UA/call state; callbacks revoke without waiting on their own lease.
  REGISTER_OK and call admission cross-check native UA registration. Focused
  older fake-SDK fixtures explicitly model successful registrar confirmation;
  PR31 tests retain negative native-registration and teardown-race coverage.
- Keep both SDK integrity script families and ABI probes; rename the overlapping
  legacy CTest so each gate executes. Optional native builds require both
  Baresip and libre inputs. Native SDK code remains isolated from the GUI.
- Apply PR36's viewport scrolling on top of the latest DPI/accessibility UI.
  Keep PR39's batch and dialing matrix regression targets and equivalent bounds
  checks in both projection paths. Every test is registered before gate labels
  are assigned.
- Explicitly declare the GUI's Windows 10 API minimum. MinGW additionally selects
  Unicode entry points and links ksuser for the audio-format GUIDs. MSVC startup
  and library settings remain intact.

## Verification

- Host CMake configuration with WIN32=TRUE for portable policy test targets.
  CTest: **61/61 passed**, including 26 integration and 35 unit tests and the
  offline SDK provenance/integrity scripts.
- Host exclusions: ui_smoke, ui_blackbox, windows_process, audio_inventory,
  audio_test_tone, baresip_modules; these require Windows APIs or executables.
- Full Windows cross-build with llvm-mingw 20261006 (x86_64 UCRT), including GUI,
  all 67 registered test executables/scripts and helper programs: **passed**.
- git diff --check: passed; all seven PR heads are ancestors of this branch.

Cross-compilation does not execute Windows tests. Windows CI remains required
for UI smoke/blackbox and native Windows process/audio tests. No actual audited
Baresip/libre SDK, SIP server, call, RTP/SRTP or physical headset was used; these
merges do not establish production readiness or real calling support.
