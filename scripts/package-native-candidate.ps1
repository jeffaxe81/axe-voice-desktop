[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$Root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Prefix=Join-Path $Root 'build/native-sdk-tls-media/prefix'
$Binaries=Join-Path $Root 'build/native-sdk-tls-media/product/Release'
$Stage=Join-Path $Root 'build/native-candidate-stage'
$Output=Join-Path $Root 'build/native-candidate-installer'
# Reject stale ABI-only provenance or an SDK-free product cache before staging.
$Manifest=Get-Content (Join-Path $Prefix 'CANDIDATE-MANIFEST.json') -Raw | ConvertFrom-Json
$Cache=Get-Content (Join-Path $Root 'build/native-sdk-tls-media/product/CMakeCache.txt') -Raw
if($Manifest.gui_native_sdk -ne 'ON/native candidate' -or $Manifest.tls -ne $true -or
   $Manifest.production_approved -ne $false -or $Cache -notmatch '(?m)^AXV_ENABLE_NATIVE_SIP:BOOL=ON\r?$') {
    throw 'Candidate provenance does not match the native TLS GUI build'
}
New-Item -ItemType Directory -Force $Stage,$Output | Out-Null
Copy-Item (Join-Path $Binaries 'AXE-Voice.exe') $Stage
Get-ChildItem (Join-Path $Prefix 'bin') -Filter '*.dll' | Copy-Item -Destination $Stage
Copy-Item (Join-Path $Prefix 'licenses') $Stage -Recurse -Force
Copy-Item (Join-Path $Prefix 'CANDIDATE-MANIFEST.json') $Stage
Copy-Item (Join-Path $Root 'installer/CANDIDATE-README.txt') $Stage
# Ship the redistributable app-local CRT: a per-user installer cannot assume
# administrative installation of the Visual C++ runtime on the target PC.
$Vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$VsRoot=(& $Vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath).Trim()
$Crt=Get-ChildItem (Join-Path $VsRoot 'VC/Redist/MSVC') -Directory | Sort-Object Name -Descending | ForEach-Object {
    $Candidate=Join-Path $_.FullName 'x64/Microsoft.VC143.CRT'
    if(Test-Path $Candidate) {$Candidate}
} | Select-Object -First 1
if(!$Crt) {throw 'MSVC app-local redistributable runtime not found'}
Get-ChildItem $Crt -Filter '*.dll' | Copy-Item -Destination $Stage
$Compiler=Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6/ISCC.exe'
if(!(Test-Path $Compiler)) {throw 'Inno Setup 6 is required to package the candidate'}
& $Compiler "/DCandidateSource=$Stage" "/DCandidateOutput=$Output" (Join-Path $Root 'installer/axe-native-candidate.iss')
if($LASTEXITCODE -ne 0) {throw 'Candidate installer compilation failed'}
$Installer=Join-Path $Output 'AXE-Voice-Native-Candidate-Setup.exe'
$Target=Join-Path $Root 'build/native-candidate-install-check'
$Install=Start-Process -FilePath $Installer -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/DIR=`"$Target`"") -Wait -PassThru
if($Install.ExitCode -ne 0) {throw 'Candidate silent installation failed'}
try {
    & (Join-Path $Target 'AXE-Voice.exe') --native-ui-smoke
    if($LASTEXITCODE -ne 0) {throw 'Installed candidate GUI smoke failed'}
    foreach($File in (Get-ChildItem $Stage -File)) {
        $Installed=Join-Path $Target $File.Name
        if((Get-FileHash $File.FullName).Hash -ne (Get-FileHash $Installed).Hash) {throw "Installed file differs: $($File.Name)"}
    }
} finally {
    $Uninstall=Start-Process -FilePath (Join-Path $Target 'unins000.exe') -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART') -Wait -PassThru
    if($Uninstall.ExitCode -ne 0) {throw 'Candidate silent uninstall failed'}
}
if(Test-Path (Join-Path $Target 'AXE-Voice.exe')) {throw 'Candidate executable remained after uninstall'}
Get-FileHash $Installer -Algorithm SHA256 | Format-List
