# AXE Voice Desktop — v0.14.0 codec dependencies (public checkpoint)

**Scope:** public documentation only. Proprietary AXE Voice source and binaries remain OUTSIDE the MicroSIP/GPL research repository.

## Implemented in the separate proprietary source package v0.14.0

- External codec catalogue: G.711 PCMA/PCMU, Opus, G.722, GSM 06.10, Speex, and G.729.
- Optional Windows **vcpkg manifest** for permissively licensed **libopus and libspeex** SDKs, with an opt-in CMake symbol-link smoke test.
- Offline module/library inventory, checks for PE64/ELF64 candidates, report as JSON, and generation of a candidates-only Baresip module fragment. **No module is called working solely because a file exists.**
- GSM requires external libgsm and a separately verified Baresip-compatible codec module. Speex requires a compatible module too.
- Bcg729 G.729 is never linked or bundled by default; a suitable commercial license and compatible module must be obtained for a proprietary app. GPLv3 distribution is not assumed compatible.
- The unauthenticated ctrl_tcp sample directives were commented out by default.

## Verification

- Linux CMake Release: 23/23 CTest passed.
- Linux Clang 17 ASan/UBSan: 23/23 CTest passed.
- 8 Python codec policy unit cases passed.
- Windows MSVC builds, actual dependency installation, ABI loading, live calls, SIP/SDP/RTP/SRTP, licensed commercial distribution: NOT verified.
- Commercial release still blocked; no Baresip engine bundled.

## Third-party licensing references

- https://github.com/baresip/baresip/wiki/Modules
- https://github.com/baresip/baresip/wiki/G.729-codec
- https://github.com/xiph/opus
- https://github.com/xiph/speex
- https://github.com/BelledonneCommunications/bcg729

**No third-party library binaries or proprietary code is published in this PR.**
