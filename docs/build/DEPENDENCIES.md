# Build Dependencies — MicroSIP 3.22.16.0

## Source of truth

This inventory was extracted from the official MicroSIP 3.22.16 source archive imported under:

`upstream/microsip-3.22.16.0/MicroSIP-3.22.16-src/`

The original `microsip.vcxproj` must remain unchanged in the immutable upstream tree.

## Project type and platforms

- C/C++ Windows desktop application
- MFC linked statically
- Unicode
- Configurations:
  - Debug / Release
  - Win32
  - x64
  - ARM64

## PJSIP tree required

The MicroSIP project imports:

`..\build\vs\pjproject-vs14-common-config.props`

and includes headers from:

- `..\pjlib\include`
- `..\pjlib-util\include`
- `..\pjsip\include`
- `..\pjmedia\include`
- `..\pjnath\include`

This means the MicroSIP project is expected to be placed inside, or immediately below, a compatible PJSIP/pjproject source tree.

### Candidate PJSIP baseline

The MicroSIP changelog records **PJSIP 2.15.1** beginning with the 3.22.3 series. No later 3.22.x changelog entry found during this assessment reports another PJSIP upgrade.

Therefore, **PJSIP 2.15.1 is the current build candidate**, but it remains a build hypothesis until the 3.22.16 source is compiled successfully against that exact tag.

The official PJSIP 2.15.1 `version.mak` confirms version 2.15.1. Its Visual Studio common config uses toolset `v140`.

## Third-party paths referenced by MicroSIP

The official `microsip.vcxproj` references:

- SDL
- SQLiteCpp
- OpenSSL
- IPP
- Opus
- SILK
- FFmpeg
- libvpx
- OpenCORE AMR-NB
- vo-amrwbenc
- OpenCORE AMR-WB
- bcg729
- PJSIP-generated/common libraries

## Explicit link dependencies

The project explicitly requests:

- `chkstk_ms.lib`
- `bcg729.lib`
- `opus.lib`
- `Wtsapi32.lib`

Additional libraries are expected to come from the PJSIP build and the referenced third-party directories.

## Windows SDK / compiler observations

The official MicroSIP project contains a Windows SDK include path for:

`Windows Kits\10\Include\10.0.19041.0\winrt`

The PJSIP 2.15.1 Visual Studio property sheet sets:

- default/build toolset: `v140`

This combination must be reproduced first. Modernizing to a newer MSVC toolset is a separate change and must not be mixed into the upstream reproduction step.

## Reproduction strategy

1. Keep the official MicroSIP tree immutable.
2. Obtain PJSIP 2.15.1 from the official `pjsip/pjproject` repository/tag.
3. Create a build workspace with the directory relationship expected by `microsip.vcxproj`.
4. Inventory which third-party components are bundled by PJSIP and which must be obtained separately.
5. Reproduce the expected libraries for one target first: **Release x64**.
6. Build MicroSIP without branding or functional changes.
7. Only after a successful original build create the working/customized source tree.

## First build target

**Release | x64**

Win32 and ARM64 remain supported targets in the upstream project but are not the first validation target.

## Open questions

- Confirm whether stock PJSIP 2.15.1 is sufficient or whether MicroSIP relies on project-specific PJSIP patches.
- Confirm exact versions/build flags for OpenSSL, FFmpeg, libvpx, SQLiteCpp, bcg729 and other codecs.
- Confirm whether `v140` can be reproduced in the selected build environment without changing upstream files.
