#ifndef CandidateSource
  #error CandidateSource must point to the verified candidate staging directory
#endif
#ifndef CandidateOutput
  #error CandidateOutput must be provided
#endif
[Setup]
AppId=AXEVoiceNativeCandidate
AppName=AXE Voice Native Candidate
AppVersion=0.73.0
AppPublisher=AXE Voice
DefaultDirName={localappdata}\Programs\AXEVoiceNativeCandidate
DefaultGroupName=AXE Voice Native Candidate
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#CandidateOutput}
OutputBaseFilename=AXE-Voice-Native-Candidate-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\AXE-Voice.exe
CloseApplications=no
DisableProgramGroupPage=yes
[Files]
Source: "{#CandidateSource}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\AXE Voice Native Candidate"; Filename: "{app}\AXE-Voice.exe"
[Registry]
; Per-user URL protocol. The running app always confirms external requests.
Root: HKCU; Subkey: "Software\Classes\axvoice"; ValueType: string; ValueName: ""; ValueData: "URL:AXE Voice Click-to-Call"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\axvoice"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\axvoice\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\AXE-Voice.exe"" ""%1"""

[Run]
Filename: "{app}\AXE-Voice.exe"; Description: "Abrir AXE Voice candidato"; Flags: nowait postinstall skipifsilent

[Code]
#include "native-upgrade-guard.iss"
