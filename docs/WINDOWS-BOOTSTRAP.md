# AXE Voice — Windows desktop bootstrap (v0.16 development)

## Scope
A small **Windows GUI shell** built with the native Win32 API. This is intentionally not a working SIP softphone yet. The call button displays a truthful "Baresip integration pending" message and never places calls.

The repository's previous MicroSIP/PJSIP README is historical. The intended next audio engine is Baresip, subject to license/dependency validation. No real SIP credentials are embedded.

## Build
From **x64 Native Tools Command Prompt for VS 2022** or PowerShell with CMake + MSVC correctly installed:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 4
Get-ChildItem -Path .\build -Filter AXE-Voice.exe -Recurse
& ".\build\Release\AXE-Voice.exe"
```

Note: The output above assumes the Visual Studio generator. If CMake uses a single-configuration generator, locate the executable with Get-ChildItem.

## Acceptance criteria
- CMake configures a Windows x64 project with target AXEVoice.
- Release build produces AXE-Voice.exe.
- Window displays number/SIP URI field and "Chamar".
- Empty number yields a validation message.
- Nonempty number explicitly says Baresip is not yet integrated.
- Closing the window exits cleanly.
- No SIP signaling, microphone or remote provisioning is claimed in this milestone.

## Next implementation
Introduce a separate SIP backend interface, integrate supported Baresip/re libraries and their licensing, register SIP account, establish outgoing/incoming calls, manage audio devices, and run end-to-end call tests. Never store account passwords in git.
