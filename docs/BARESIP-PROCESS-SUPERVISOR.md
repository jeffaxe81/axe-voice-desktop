# Windows Baresip process supervision

The Win32 `WindowsProcess` class starts an **explicit absolute binary path** using `CreateProcessW`, not a shell.
It creates the process suspended, assigns it to a Windows Job Object with `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`, then resumes it. This ensures the process tree is terminated on AXE Voice shutdown.

The initial implementation intentionally passes **no SIP passwords or user-defined command-line arguments**, and never claims that process start means SIP registration.

A UI button may launch only the `baresip.exe` next to the AXE Voice executable, after **explicit user confirmation**. Users must verify the publisher/hash/source themselves before opting in. This bootstrap does not authenticate binaries.

**Important:** Stop uses forced job termination, not a graceful Baresip hangup. Do not use this build for production calls. A future authenticated Baresip control channel must send a graceful quit and drain current calls before forcing shutdown.

## Validation
- Windows CI tests a real child process start, duplicate start rejection, stop, and restart.
- The test child is an inert 30-second sleeper and makes no SIP calls.
- GUI launch tests and PABX registration are separate acceptance criteria.

For actual Baresip config files, the upstream supports the `-f` configuration-directory option; this bootstrap does not yet pass it. See official upstream documentation: https://github.com/baresip/baresip/wiki/Configuration

## Laboratory profile (0.16.3)

The GUI now runs Baresip with a dedicated -f directory under the current user's LocalAppData/AXEVoice/baresip-lab, rather than consuming the default Baresip account profile. Before each start, it rejects configured SIP accounts, certain unauthenticated control modules, symlinks and Windows reparse points for the profile files. See [BARESIP-LAB-PROFILE.md](BARESIP-LAB-PROFILE.md). This is not a comprehensive trust, ACL or configuration audit.
