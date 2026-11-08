#ifndef WinUI3Source
  #error WinUI3Source must point to the verified WinUI 3 native laboratory package
#endif
#ifndef WinUI3Output
  #error WinUI3Output must be provided
#endif
[Setup]
; Separate AppId prevents an upgrade or uninstall from touching AXE Voice Win32.
AppId=AXEVoiceWinUI3NativeLabCandidate
AppName=AXE Voice WinUI 3 Native Lab
AppVersion=0.83.0
AppPublisher=AXE Voice
DefaultDirName={localappdata}\Programs\AXEVoiceWinUI3NativeLab
DefaultGroupName=AXE Voice WinUI 3 Native Lab
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
OutputDir={#WinUI3Output}
OutputBaseFilename=AXE-Voice-WinUI3-Native-Lab-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=no
DisableProgramGroupPage=yes
SetupIconFile={#WinUI3Source}\Assets\axe-voice.ico
UninstallDisplayIcon={app}\Assets\axe-voice.ico
Uninstallable=yes
CreateUninstallRegKey=yes
UsePreviousAppDir=no
[Files]
Source: "{#WinUI3Source}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Tasks]
Name: "desktopicon"; Description: "Criar atalho na área de trabalho"; Flags: unchecked
[Icons]
Name: "{group}\AXE Voice WinUI 3 Native Lab"; Filename: "{app}\AXEVoice.WinUI3.exe"; IconFilename: "{app}\Assets\axe-voice.ico"
Name: "{autodesktop}\AXE Voice WinUI 3 Native Lab"; Filename: "{app}\AXEVoice.WinUI3.exe"; IconFilename: "{app}\Assets\axe-voice.ico"; Tasks: desktopicon
; Deliberately no [Run] hook or automatic SIP registration. This lab candidate
; needs a compatible .NET 10 runtime and Windows App Runtime 2.6.5.
