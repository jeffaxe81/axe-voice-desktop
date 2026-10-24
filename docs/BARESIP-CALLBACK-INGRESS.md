# Baresip native callback ingress — v0.38 (laboratório)

`src/baresip_callback_ingress.h` introduces a thread-safe adapter-side callback
transport that **is not connected to the Win32 application or Baresip SDK**.
It only transfers abstract events, not trust. The existing UI still selects
`UnavailableSipEngine`, reports no SIP registration, and disables dialing.

## Contract for a future audited native Baresip integration

1. Bind each native callback to the session token returned by `begin_session()`.
   Pass that captured token to every callback operation. Never ask for the current
   session from inside a delayed callback.
2. The adapter must prove callback provenance independently. The ingress cannot
   produce `VerifiedNativeApi` and must never be used to infer proof from stdout,
   process activity or file existence.
3. Assign strictly increasing, unique, nonzero native call keys within the
   generation; retain a key through establishment and closure. Never pass a
   pointer, SIP URI, dialed number or credential into the mailbox. The ingress
   maps this key into a separate monotonic opaque call token.
4. Only one active call is supported by this laboratory bridge. A duplicate,
   wrong call key, unauthorized event kind or queue failure revokes the session
   and invalidates pending events. Delayed callbacks from an earlier generation
   are quietly rejected without invalidating the newer session.
5. After `revoke()`, the integration must unregister callbacks and wait for
   callback completion before destroying native resources. The class itself
   does not own native resources and cannot guarantee C API lifetime safety.
6. A single event batch is limited to 32 events. Overflow invalidates the
   transport until a new generation. Ordering of callbacks is a responsibility
   of the native adapter, not a guarantee supplied by a mutex alone.

## Tests and remaining gates

`baresip_callback_ingress` covers registration events, call token mapping,
closed calls, late callbacks, concurrency across retired/current generations,
native key reuse, revoke and bounded queue overflow. All source events in the
test are synthetic. CI Windows compiles the code without linking Baresip.

Still required: audited SDK versions and license review; real event handler
registration/unregistration; authenticated session/provenance issuance; SIP
account provisioning, registration and calls; TLS/SRTP and headset media
tests; Windows 10/11 manual QA. Keep PR Draft until these are implemented.
