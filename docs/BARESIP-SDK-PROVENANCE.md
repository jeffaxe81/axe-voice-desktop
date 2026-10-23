# AXE Voice — Baresip SDK provenance gate (v0.45, laboratório)

The native Baresip SDK bridge remains an **optional isolated static library**.
It is NOT linked into `AXE-Voice.exe` and does not enable SIP registration,
actual calls, media, credentials, or `SipEvidence::VerifiedNativeApi`.

## New mandatory checks

Enabling `AXV_BUILD_NATIVE_BARESIP_ADAPTER` requires these four CMake inputs:

- `AXV_BARESIP_INCLUDE_DIR`: directory containing `baresip.h`.
- `AXV_BARESIP_LIBRARY`: location of the approved SDK library.
- `AXV_BARESIP_HEADER_SHA256`: independently approved 64-hex SHA-256 value.
- `AXV_BARESIP_LIBRARY_SHA256`: independently approved 64-hex SHA-256 value.

The configure step reads the **actual files**, rejects nonfiles, missing or
invalid hashes, and rejects content that differs from the expected hashes.
The gate runs before the optional native adapter target is declared.

Windows PowerShell example after independent source/license review:

```powershell
Get-FileHash C:\SDK\baresip\include\baresip.h -Algorithm SHA256
Get-FileHash C:\SDK\baresip\lib\baresip.lib -Algorithm SHA256

cmake -S . -B build-native -G "Visual Studio 17 2022" -A x64 `
  -DAXV_BUILD_NATIVE_BARESIP_ADAPTER=ON `
  -DAXV_BARESIP_INCLUDE_DIR="C:/SDK/baresip/include" `
  -DAXV_BARESIP_LIBRARY="C:/SDK/baresip/lib/baresip.lib" `
  -DAXV_BARESIP_HEADER_SHA256="<approved_header_sha256>" `
  -DAXV_BARESIP_LIBRARY_SHA256="<approved_library_sha256>"
```

**Do not accept a hash merely because an unknown downloaded file reports
that hash.** Pin upstream source commit/tag, origin, dependency lockfile,
toolchain and package provenance independently. Record binary and source
licensing review. SHA-256 equality establishes file identity only; it is not
publisher authentication or a proof of runtime security.

The gate currently covers `baresip.h` and one supplied SDK library. It does
not recursively attest other SDK headers, librem, codecs, DLLs or transitive
libraries. These must be audited separately before a real engine is enabled.

## CI and negative regression tests

`baresip_sdk_provenance` uses a **fake test header** and a synthetic
text fixture pretending to be a library path; it never loads the SDK.
The CMake script verifies approved/matching hashes, hexadecimal case,
missing hash, invalid length, missing file, directory instead of library,
modified library and modified header. Every intentionally invalid case must
be rejected. This is an integration quality gate, not SIP interoperability.

## Next gates before native calls

- Audit/pin a matching Baresip and libre SDK release, ABIs, dependency
  versions, checksums, and license obligations.
- Implement native event-loop ownership and callback quiescence using the
  isolated adapter, then integrate an actual `SipEngine`.
- Protect account secrets; perform registration with a test PABX; validate
  TLS/SRTP, incoming/outgoing calls, headset audio, reconnection and teardown.
- Keep PR Draft and `main` unchanged until hardware and Windows 10/11
  acceptance tests succeed.
