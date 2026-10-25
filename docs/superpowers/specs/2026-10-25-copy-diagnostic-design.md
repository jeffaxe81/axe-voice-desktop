# Candidate0.56 — copy local diagnostic

Homologation on the Dígitro PBX, physical headset audio and route migration remain pending. Continue the previously recorded diagnostic-copy feature from public requirements only; do not port private/historical implementations.

Add COPIAR STATUS on Audio and Ctrl+Shift+D from any page. Copy the existing readiness report as Windows CF_UNICODETEXT with CRLF and a NUL terminator. The formatter accepts only CallReadiness booleans; no account, destination, credentials, device identifiers or raw SDK logs can enter it. Include a fixed candidate version and explicit pending physical acceptance notice. The clipboard persists until Windows/another application replaces it; explain this in help.

Approaches: a disk report adds file permissions and cleanup without helping a quick support exchange; an explicit local clipboard action uses existing readiness and needs no network. Choose clipboard. No automatic copy, disk writes, SIP connection or change to call state. Failure is shown and may be retried by the user. Allocate/lock/fill before opening/emptying clipboard, unlock before transfer, close on all opened paths, free memory until SetClipboardData succeeds. EmptyClipboard can succeed before a later transfer fails; no preservation guarantee is claimed.

Smoke modes build the report and exercise button/accelerator command routing without touching system clipboard or audio. Portable formatting tests and isolated WinAPI test stubs exercise ownership/failure paths; physical Windows clipboard/keyboard integration remains pending. Package0.56.0 after Windows and native installer gates.
