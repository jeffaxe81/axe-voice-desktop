# SIP engine boundary

The `SipEngine` interface is the contract for SIP registration and voice calls. The current `UnavailableSipEngine` is intentionally non-operational: it cannot register or dial. UI messages must not report success.

Next work: integrate a supported, pinned version of Baresip/re into an implementation of this interface, handle asynchronous SIP events safely on the UI thread, secure credential storage, account validation, and audio device selection. Validate TLS/SRTP policies with an actual test server and record logs without secrets.

No passwords or tokens belong in source code or unencrypted configuration. A built GUI alone is not evidence of registration or media flow.
