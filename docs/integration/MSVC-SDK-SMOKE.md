# MSVC SDK candidate smoke

`scripts/build-native-sdk-smoke.ps1` builds pinned upstream libre and Baresip4.12.0 with Visual Studio2022 x64. Commits are checked, tracked source changes are rejected, and two candidate profiles are built: `abi` without TLS/audio, and `tls-media` with pinned OpenSSL3.5.7 plus static G.711/WASAPI modules. Neither profile is approved for production.

The script computes hashes of the resulting real `.lib` and headers, passes them to the existing native preflights, builds both ABI probes and the real libre reactor smoke. The reactor is executed in both profiles: no Baresip UA, account, registration, SIP call or media session is created. The GUI remains unlinked from the native SDK.

A dedicated Windows2022 workflow runs both profiles and uploads candidate headers/libraries with commit/hash manifest and both upstream license texts. Candidate hashes identify the exact CI outputs; they do not provide independent supplier authentication or production approval. The probe executables are artifacts for ABI inspection; the ABI executables are not run by the workflow.

Expected gate: actual MSVC compile/link and real reactor exit0 in Windows. General GUI/unit/blackbox/integration CI remains a separate check. Production still needs actual TLS handshake/certificate validation, device and codec runtime validation, callback quiescence, account credentials handling, PABX registration, bidirectional media and hardware confirmation.

Actual MSVC execution also exposed the upstream C `struct log` / global C++ math-template name collision and SDK logging macros expanding inside STL pragmas. `baresip_sdk_headers.h` scopes the SDK's C log tag and restores caller macros around the SDK include. Consumed SDK call/event ABI types and extern-C symbols remain unchanged; installed SDK header bytes are not modified. The real ABI probe rejects leaked logging macros during compilation.

The `-TlsMedia` profile compiles OpenSSL from commit `8cf17aaeb4599f8af87fefd810b5b5fee90fe69e` using the MSVC shared runtime. A local probe links G.711/WASAPI exports and creates an OpenSSL client context with TLS1.2 minimum and peer verification configured. It does not perform a handshake, validate a certificate, initialize audio modules, open devices or exchange media. Matching OpenSSL DLLs, license and file hashes accompany the candidate artifact. `AXV_NATIVE_TLS_MEDIA` defaults OFF and does not connect the SDK to the GUI.
