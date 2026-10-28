#ifndef CandidateSource
  #error CandidateSource must point to the verified candidate staging directory
#endif
#ifndef CandidateOutput
  #error CandidateOutput must be provided
#endif
[Setup]
AppId=AXEVoiceNativeCandidate
AppName=AXE Voice Native Candidate
AppVersion=0.65.0
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
[Run]
Filename: "{app}\AXE-Voice.exe"; Description: "Abrir AXE Voice candidato"; Flags: nowait postinstall skipifsilent
