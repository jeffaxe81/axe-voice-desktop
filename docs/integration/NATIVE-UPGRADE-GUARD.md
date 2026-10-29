# Native installation and startup guard

## Integrated release and recorded evidence

Candidate0.68 was merged in PR #154 at
`68c7bf56d0e47bed8a490cb2b32fe9eb8d5aa8f8`. The exact tested head was
`c35e0f8cdd4c9c8d3e346261fa80746fcfc1e180`.

- [Windows CI40943593634](https://github.com/jeffaxe81/axe-voice-desktop/actions/runs/40943593634):115/115 passed.
- [Native SDK40943593692](https://github.com/jeffaxe81/axe-voice-desktop/actions/runs/40943593692): both SDK profiles passed, including Inno compilation and installed held-marker, legacy sharing-conflict, live GUI, refusal with unchanged payload hashes, successful retry, reciprocal startup and uninstall checks.
- Installer artifact ID: `12927568379`.

The0.68.1 work is additional specialist validation in progress, not a passed
release or a product version bump. Interactive postinstall `[Run]` handoff,
actual global teardown, and current production-Inno races remain pending.
These recorded automated results do not establish physical hardware, cross-session
or cross-account acceptance, signing, Dígitro, or headset acceptance; the external
acceptance work remains pending at the user's request.

## Contract

The guarded native application and candidate installer cooperate to prevent
installation or uninstallation from replacing files while a guarded native
application is running. Setup refuses to proceed rather than closing the app,
ending a call, or waiting for calls to finish. A native app starting while setup
holds its reservation refuses startup before creating the GUI or starting SIP.
Multiple native application instances remain supported.

This protocol applies to the native SDK build. The SDK-free laboratory GUI does
not participate. Historical application versions without this guard do not
publish these markers. A conservative file probe supplements the protocol for
those versions, as described below; they must still be closed before upgrading
or uninstalling.

## Named-object protocol

Both sides derive matching `Global\` mutex names for the candidate product,
with separate `Running` and `Setup` markers and the process token's `TokenUser`
SID as the owner scope. The session identifier is deliberately absent: the same
Windows account in another session must participate in the same guard. Another
account has a different scope. These names are cooperation markers, not a
security boundary against a hostile process able to create those objects.

The app first creates and retains a running marker, allowing an existing running
marker, then probes for the setup marker. Setup first creates and retains its
setup marker, rejecting an existing setup marker, then probes for the running
marker. A marker's existence is what matters; mutex ownership is not used to
make the application single-instance. Every accepted app retains its own handle,
so setup remains blocked until the final participating app releases its handle.

Inno Setup's built-in `AppMutex` check alone is insufficient: checking for an
application before publishing the setup reservation leaves a startup race.
The reciprocal publication order is part of the contract.

Creation precedes the reciprocal probe on both sides. Consequently simultaneous
startup cannot admit both sides: either one is refused or both are refused.
Only a probe reporting `ERROR_FILE_NOT_FOUND` establishes absence. Access
denied, incompatible named-object collisions, token/SID failures, and other
creation or probe errors refuse the operation. Errors must not be interpreted
as permission to mutate installed files or start the native runtime.

## Lifetime and installer handoff

The native guard is acquired at the beginning of `wWinMain`. Its global RAII
holder precedes the native engine and runtime globals in declaration order, so
its destructor runs after their destruction. Returning from the message loop or
destroying the main window must not release protection before native SDK,
reactor, and player teardown. Rejected startup releases any partial acquisition.

Installer and uninstaller initialization reserve setup before checking for apps.
The installer retains the reservation through installation mutations, then
releases it at `ssPostInstall` before the existing `[Run]` launch. Otherwise the
new app would reject the installer's own launch. Deinitialization cleanup is
idempotent and handles early rejection, cancellation, and failure. Uninstall
retains its reservation through removal and releases it during deinitialization.

## Conservative legacy fallback

Before installation mutations in `PrepareToInstall`, and before admitting
uninstallation in `InitializeUninstall`, setup probes the existing target
`AXE-Voice.exe` with `CreateFileW`, writable access, zero sharing, and
`OPEN_EXISTING`. This probes an existing file without creating or truncating it
and without writing any bytes. A locked executable or another probe failure
refuses the operation before installed payload changes. An absent executable
must be distinguished from an access or sharing error.

The successful probe handle is closed; it is not a reservation that old apps
honor. A legacy app can start after the probe, so this fallback does not guarantee
race protection for historical versions. Only cooperating apps participate in
the reciprocal startup protocol. A lock refusal must leave the installed payload
unchanged and must not terminate the app or its calls.

## Validation and limits

Validation must cover multiple running holders, simultaneous app/setup startup,
setup/setup contention, failure cleanup, access-denied probes, named-object type
collisions, and marker lifetime through global teardown. Installer integration
must also verify silent operation, uninstall refusal, cancellation, and the
postinstall launch handoff. Test presence alone is not evidence that these checks
have passed; record actual commands and results with the change.

The legacy regression holds the target executable open for reading with sharing
that denies writes, without publishing a Running marker. Upgrade and uninstall
must refuse with unchanged payload hashes, then an upgrade must succeed after
the handle closes. This models a sharing conflict; it does not establish the
behavior of every historical executable or close its startup race.

Physical Windows checks across sessions and user accounts remain pending unless
separately recorded with evidence. They must confirm same-SID cross-session
blocking and different-SID isolation, including the relevant token and ACL
behavior. Native SDK behavior and active-call preservation require their own
runtime evidence; an SDK-free helper does not establish them.
Signing, physical Windows hardware, and Dígitro/headset acceptance remain pending
unless separately recorded with evidence.

The guard does not make an upgrade atomic, implement rollback, validate the
payload, or remove obsolete files from older installations. Obsolete-file
cleanup remains separate pending work. It also does not protect arbitrary custom
installation directories shared by different accounts, because its scope is the
owner SID rather than the target path.
