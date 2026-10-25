# Engine lifecycle contract

The engine lifecycle state machine is pure logic, not an active process supervisor.

Supported states: Stopped → Starting → Running → Stopping → Stopped; failures move Starting/Running/Stopping into Failed. A failed state must be stopped explicitly before a restart. Unsupported events leave state unchanged.

## Next implementation gate

Integrate with an audited Baresip runtime and a Win32-owned worker/process handle. Ensure cleanup on window shutdown, timeouts, no command injection, no credentials on command lines, and log redaction. A file existing at `baresip.exe` does not confer permission to execute it.

A successful state-transition test proves only transition rules; it does not demonstrate process creation, SIP registration, or audio.
