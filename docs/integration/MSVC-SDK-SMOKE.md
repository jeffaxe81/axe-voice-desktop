# MSVC SDK candidate smoke

`scripts/build-native-sdk-smoke.ps1` builds pinned upstream libre and Baresip4.12.0 with Visual Studio2022 x64. Commits are checked, tracked source changes are rejected, and the candidate is built without OpenSSL, TLS or audio modules. This is an ABI/reactor-only profile; it cannot satisfy the production TLS/media gates.

The script computes hashes of the resulting real `.lib` and headers, passes them to the existing native preflights, builds both ABI probes and the real libre reactor smoke. Only the reactor is executed: no Baresip UA, account, registration, SIP call or media session is created. The GUI remains unlinked from the native SDK.

A dedicated Windows2022 workflow runs this profile and uploads candidate headers/libraries with commit/hash manifest and both upstream license texts. Candidate hashes identify the exact CI outputs; they do not provide independent supplier authentication or production approval. The probe executables are artifacts for ABI inspection; the ABI executables are not run by the workflow.

Expected gate: actual MSVC compile/link and real reactor exit0 in Windows. General GUI/unit/blackbox/integration CI remains a separate check. Production still needs an appropriate TLS/media SDK, actual callback quiescence, account credentials handling, PABX registration, bidirectional media and hardware confirmation.
