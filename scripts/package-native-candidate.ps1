[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$Root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Prefix=Join-Path $Root 'build/native-sdk-tls-media/prefix'
$Binaries=Join-Path $Root 'build/native-sdk-tls-media/product/Release'
$Stage=Join-Path $Root 'build/native-candidate-stage'
$Output=Join-Path $Root 'build/native-candidate-installer'
# Recursive enumeration must not skip directories later followed by staging.
$SdkDirectory = Get-Item -LiteralPath $Prefix -Force
for ($Ancestor = $SdkDirectory; $Ancestor -and $Ancestor.FullName -ne $Root; $Ancestor = $Ancestor.Parent) {
    if ($Ancestor.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'SDK redirected paths are not allowed' }
}
foreach ($Item in (Get-ChildItem -LiteralPath $Prefix -Recurse -Force)) {
    if ($Item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'SDK redirected paths are not allowed' }
}
# Reject stale ABI-only provenance or an SDK-free product cache before staging.
$Manifest=Get-Content (Join-Path $Prefix 'CANDIDATE-MANIFEST.json') -Raw | ConvertFrom-Json -NoEnumerate
# Reject collection/string coercion before comparing provenance or hashes.
if ($null -eq $Manifest -or $Manifest.GetType() -ne [System.Management.Automation.PSCustomObject] -or $Manifest.gui_native_sdk -isnot [string] -or
    $Manifest.tls -isnot [bool] -or $Manifest.production_approved -isnot [bool] -or
    $Manifest.product_version -isnot [string] -or $Manifest.product_sha256 -isnot [string] -or
    $null -eq $Manifest.files -or $Manifest.files.GetType() -ne [System.Management.Automation.PSCustomObject]) { throw 'Candidate manifest has invalid field types' }
$Cache=Get-Content (Join-Path $Root 'build/native-sdk-tls-media/product/CMakeCache.txt') -Raw
if($Manifest.gui_native_sdk -ne 'ON/native candidate' -or $Manifest.tls -ne $true -or
   $Manifest.production_approved -ne $false -or $Cache -notmatch '(?m)^AXV_ENABLE_NATIVE_SIP:BOOL=ON\r?$' -or
   $Cache -notmatch '(?m)^AXV_NATIVE_TLS_MEDIA:BOOL=ON\r?$') {
    throw 'Candidate provenance does not match the native TLS GUI build'
}
# Validate provenance before copying: a valid old manifest must not bless a
# replaced GUI or dependency. Include files recursively in the install check.
$Version = [regex]::Match($Cache, '(?m)^CMAKE_PROJECT_VERSION:STATIC=([^\r\n]+)\r?$').Groups[1].Value
$InstallerSpec = Get-Content (Join-Path $Root 'installer/axe-native-candidate.iss') -Raw
$InstallerVersion = [regex]::Match($InstallerSpec, '(?m)^AppVersion=([^\r\n]+)\r?$').Groups[1].Value
if (!$Version -or $Manifest.product_version -ne $Version -or $InstallerVersion -ne $Version) {
    throw 'Candidate product, manifest and installer versions differ'
}
if ($Manifest.product_sha256 -notmatch '^[0-9a-f]{64}$' -or
    (Get-FileHash -LiteralPath (Join-Path $Binaries 'AXE-Voice.exe') -Algorithm SHA256).Hash -ne $Manifest.product_sha256) {
    throw 'Candidate GUI differs from the verified build'
}
if (!$Manifest.files -or @($Manifest.files.PSObject.Properties).Count -eq 0) { throw 'SDK file hashes missing' }
foreach ($Entry in $Manifest.files.PSObject.Properties) {
    if ($Entry.Value -isnot [string]) { throw 'SDK manifest hash must be a string' }
    $Path = Join-Path $Prefix $Entry.Name
    if ($Entry.Value -notmatch '^[0-9a-f]{64}$' -or
        (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -ne $Entry.Value) {
        throw "SDK manifest file missing or changed: $($Entry.Name)"
    }
}
foreach ($File in (Get-ChildItem -LiteralPath $Prefix -Recurse -File -Force | Where-Object { $_.FullName -ne (Join-Path $Prefix 'CANDIDATE-MANIFEST.json') })) {
    $Relative = $File.FullName.Substring($Prefix.Length+1).Replace('\','/')
    $Expected = $Manifest.files.PSObject.Properties[$Relative].Value
    if ($Expected -notmatch '^[0-9a-f]{64}$' -or (Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256).Hash -ne $Expected) {
        throw "SDK file differs from manifest: $Relative"
    }
}
# A repeated packaging run must not retain removed DLLs or license files.
if (Test-Path $Stage) { Remove-Item $Stage -Recurse -Force }
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
    # Reinstall into the same per-user location to exercise the upgrade path.
    $Upgrade=Start-Process -FilePath $Installer -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/DIR=`"$Target`"") -Wait -PassThru
    if($Upgrade.ExitCode -ne 0) {throw 'Candidate silent upgrade failed'}
    & (Join-Path $Target 'AXE-Voice.exe') --native-ui-smoke
    if($LASTEXITCODE -ne 0) {throw 'Installed candidate GUI smoke failed'}
    foreach($File in (Get-ChildItem $Stage -Recurse -File)) {
        $Installed=Join-Path $Target $File.FullName.Substring($Stage.Length+1)
        if((Get-FileHash -LiteralPath $File.FullName).Hash -ne (Get-FileHash -LiteralPath $Installed).Hash) {throw "Installed file differs: $($File.Name)"}
    }
} finally {
    $Uninstall=Start-Process -FilePath (Join-Path $Target 'unins000.exe') -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART') -Wait -PassThru
    if($Uninstall.ExitCode -ne 0) {throw 'Candidate silent uninstall failed'}
}
foreach($File in (Get-ChildItem $Stage -Recurse -File)) {
    $Installed=Join-Path $Target $File.FullName.Substring($Stage.Length+1)
    if(Test-Path $Installed) {throw "Candidate file remained after uninstall: $($File.Name)"}
}
Get-FileHash -LiteralPath $Installer -Algorithm SHA256 | Format-List
