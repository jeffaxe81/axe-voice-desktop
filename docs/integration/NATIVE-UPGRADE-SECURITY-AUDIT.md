# Native upgrade security audit

Reviewed 2026-10-29: `installer/native-upgrade-guard.iss`,
`scripts/package-native-candidate.ps1`, `tests/test_native_upgrade_guard.ps1`,
`src/native_install_guard.h`, and the package-integrity regression scripts.
This was a read-only source audit in Linux; Windows/Inno runtime checks were
not rerun here. The reported successful candidate 0.68 CI run 40943593692 is
external validation supplied to the audit, not a run independently reproduced
by this review.

## Result

No new concrete security defect was confirmed in the reviewed upgrade guard
or package provenance gates.

- The Inno declarations consistently use the 32-bit engine ABI: TokenUser's
  SID pointer is copied as four bytes, and the UTF-16 text copy is bounded by
  the fixed 256-word destination. The token and allocated SID text are
  released in `finally`.
- Setup captures the Win32 last error immediately after mutex creation and
  opening. Only `ERROR_FILE_NOT_FOUND` permits an absent peer; creation
  failures, existing setup markers, incompatible objects, and access errors
  reject admission. Reservation release is idempotent, including failed
  uninstall initialization and setup/uninstall deinitialization.
- The legacy probe requests exclusive write access with `OPEN_EXISTING`.
  It neither creates nor truncates the executable. File/path-not-found permit
  a fresh destination; other errors reject the operation.
- SDK traversal rejects reparse points in the prefix, its ancestors below
  the repository root, and recursive children before staging. Canonical path
  checks and literal file hashes cover both manifest entries and recursively
  enumerated SDK files. Staging is cleared before copying.

## Validation limits and follow-up coverage

The legacy probe closes its handle before installation. A legacy application
that starts afterward does not participate in reciprocal marker reservation;
the existing design document explicitly records this limitation. It is not a
new issue from this audit. Historical installed files that disappear from a
new package are also outside this audit's findings and already tracked.

The Windows acceptance script covers unlocked payload markers, a legacy
sharing conflict, the live installed GUI, reciprocal launch refusal, payload
hash preservation after rejection, and successful retry. It does not exercise
interactive setup cancellation, actual setup-vs-setup competition, or an
inaccessible/incompatible setup-side named object through Inno itself. The
C++ guard test covers related object-error and cleanup behavior but is not a
substitute for those Inno paths. Suitable additional regressions would cancel
setup after reservation and verify marker disappearance and immediate GUI
launch; hold the setup marker while invoking another installer; and inject
an incompatible/denied running marker while verifying refusal without
payload changes, followed by a successful retry after removing the fixture.

The acceptance helpers dispose their `Process` objects after a timeout but
do not stop or reap the timed-out child. A timeout therefore does not prove
that subsequent package cleanup runs without a surviving installer or GUI.
This is a test-harness containment limit, not evidence that normal guard
admission fails. A regression for any future timeout cleanup change should
use an intentionally stalled child and verify that no test-owned child
remains before outer uninstall cleanup begins.
