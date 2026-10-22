# Baresip lab profile — AXE Voice 0.16.3

AXE Voice starts `baresip.exe` with `-f "<LocalAppData>/AXEVoice/baresip-lab"` on Windows. The profile directory is created on demand for the **current Windows user** and is separated from the standard Baresip profile. The GUI still asks the user to confirm execution of a locally installed, **unverified** executable.

## Safety gate (before each start)

- Reject a missing/inaccessible profile, symlinked profile, or Windows junction/reparse directory.
- Reject symlinked or non-regular `config`, `accounts`, `contacts`, and `uuid` files (including Windows reparse points).
- Refuse startup when `accounts` has any active line. Comment-only files are accepted; this intentionally prevents automatic SIP registration during laboratory bootstrap.
- Refuse startup if `config` contains an active line containing `ctrl_tcp`, `ctrl_http`, or `ctrl_ws` (unauthenticated or unevaluated remote-control modules). The check is a **guardrail**, not a full parser or guarantee against malicious configuration.
- Never pass SIP identities or passwords as process arguments; report errors without reading/printing secrets.

`-f` is a documented Baresip option: https://github.com/baresip/baresip/wiki/Configuration

## Validation

CTest `baresip_profile` checks empty profile creation, comments, configured accounts, forbidden control modules, and invalid files. Windows `windows_process` exercises `-f` with a real inert child executable and a directory containing spaces.

## Limits

This change **does not implement a SIP engine, trusted-binary verification, registration, incoming/outgoing calls, DTMF, secure credentials, media, TLS, SRTP, codecs, or authenticated IPC**. App-control modules are deliberately not activated. The UI continues to report that SIP registration is unconfirmed. The process is still forcibly stopped; use only in a laboratory until graceful call-aware shutdown exists. Directory permissions inherit the current user's profile ACLs and should be audited before credentials are stored there.
