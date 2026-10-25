# Checking Baresip on Windows

This probe checks only whether a **file** exists at a given path. It does not launch Baresip, verify its provenance, interact with a SIP server, inspect codec modules or make calls.

To inspect a separately installed Baresip manually:

```powershell
Get-Command baresip -ErrorAction SilentlyContinue
Get-ChildItem -Path C:\dev -Recurse -Filter baresip.exe -ErrorAction SilentlyContinue | Select-Object FullName
```

Do not execute unknown binaries solely based on a positive file probe. Verify publisher, release hash and licensing first. No SIP account credentials are required for this check.
