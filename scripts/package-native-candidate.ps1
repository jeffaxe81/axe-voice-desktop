[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$Root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Prefix=Join-Path $Root 'build/native-sdk-tls-media/prefix'
$Binaries=Join-Path $Root 'build/native-sdk-tls-media/product/Release'
$Stage=Join-Path $Root 'build/native-candidate-stage'
$Output=Join-Path $Root 'build/native-candidate-installer'
New-Item -ItemType Directory -Force $Stage,$Output | Out-Null
Copy-Item (Join-Path $Binaries 'AXE-Voice.exe') $Stage
Get-ChildItem (Join-Path $Prefix 'bin') -Filter '*.dll' | Copy-Item -Destination $Stage
Copy-Item (Join-Path $Prefix 'licenses') $Stage -Recurse -Force
Copy-Item (Join-Path $Prefix 'CANDIDATE-MANIFEST.json') $Stage
Copy-Item (Join-Path $Root 'installer/CANDIDATE-README.txt') $Stage
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
