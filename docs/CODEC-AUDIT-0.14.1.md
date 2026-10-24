# AXE Voice Desktop — codec artifact gate, v0.14.1

This is **public, stand-alone tooling** in the MicroSIP/GPL historical repository, not proprietary AXE Voice code. No third-party binary or voice engine is checked in. The inspector **never executes or loads a module**.

## Run (Python 3.10+; no dependencies)

```bash
python tools/codec_audit.py --modules ./external/baresip/modules --libs ./external/baresip/bin --target windows-x64 --report ./build/codec-report.json --commented-config ./build/codecs-candidates.txt --fail-on-missing-basic
python -m unittest discover -s tests -p test_codec_audit.py -v
```

Supported targets: `windows-x64` and `linux-x64`. Module artifacts expected under `--modules`; shared dependencies under optional, repeatable `--libs`. The command identifies `.dll`/`.so` headers, and direct filenames, without importing or loading them. It does not resolve transitive imports, symbol ABIs, codec payload registration, or live media negotiation.

## Outcomes

- `candidate-only` means a module file has an expected architecture; any known external library filename also exists with matching architecture. **Not ready/validated**.
- `module-missing`, `dependency-missing`, `module-architecture-mismatch`, `dependency-architecture-mismatch` block the file-level precheck.
- `custom-module-review-required` (Speex) requires an audited standalone codec module; upstream support for Speex DSP filters is not evidence of a speech codec.
- `manual-license-review-required` (G.729) always requires a separate technical/legal review; a file is **never** accepted as license evidence.
- The generated module snippet consists only of **commented** lines. It cannot activate unverified codecs or `ctrl_tcp`.
- Exit code 2 with `--fail-on-missing-basic` if any of the G.711, G.722 or Opus modules/dependencies fail the file candidate check. Nonzero exit status does not imply a call test.

## Required before commercialization

1. Establish permissive or commercial licenses for all Baresip engine, codec and integration components; GPL MicroSIP/PJSIP historical source is not automatically compatible with a proprietary build.
2. Produce Windows MSVC x64 artifacts in a reproducible build, pin dependencies, and verify all imported DLL dependencies and ABI.
3. Perform real SIP REGISTER/INVITE, SDP offer/answer, RTP/RTCP, SRTP/TLS tests on Windows with server and client codecs.
4. Validate audio in/out, codecs ordering, DTMF, call hold, transfers, NAT traversal, and device switching with logs/evidence.
5. Review G.729 third-party licensing separately and leave it disabled until approved.

Baresip references: https://github.com/baresip/baresip/wiki/Modules and https://github.com/baresip/baresip .
