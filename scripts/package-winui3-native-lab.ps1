# Build and verify the WinUI 3 native LAB installer without starting SIP.
# No credentials, tokens, registered accounts or calls are required.
[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source=Join-Path $root 'build/winui3-native-candidate'
$native=Join-Path $root 'build/native-sdk-tls-media/product/Release/axv_frontend_bridge.dll'
$prefix=Join-Path $root 'build/native-sdk-tls-media/prefix'
$output=Join-Path $root 'build/winui3-native-lab-installer'
$target=Join-Path $root 'build/winui3-native-lab-install-check'
$spec=Join-Path $root 'installer/axe-winui3-native-lab.iss'

function Require-File([string]$path) {
    if(!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing laboratory distribution file: $path" }
}
function Hash([string]$path) {
    return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
}
foreach($relative in @(
    'AXEVoice.WinUI3.exe','axv_frontend_bridge.dll','Assets/axe-voice.ico',
    'LEIA-ME-CANDIDATO.txt','licenses/baresip-LICENSE.txt',
    'licenses/libre-LICENSE.txt','licenses/OpenSSL-LICENSE.txt'
)) { Require-File (Join-Path $source $relative) }
Require-File $native
Require-File (Join-Path $prefix 'CANDIDATE-MANIFEST.json')
if((Hash (Join-Path $source 'axv_frontend_bridge.dll')) -ne (Hash $native)) {
    throw 'WinUI 3 package bridge is not the pinned native SIP build'
}
# The installed program remains explicitly a LAB candidate. Generic WinUI
# Preview bridges and SDK-free archives are not accepted here.
$manifest=Get-Content -LiteralPath (Join-Path $prefix 'CANDIDATE-MANIFEST.json') -Raw |
    ConvertFrom-Json -NoEnumerate
if($manifest.gui_native_sdk -ne 'ON/native candidate' -or
   $manifest.tls -ne $true -or $manifest.production_approved -ne $false) {
    throw 'Native SDK manifest does not authorize a WinUI 3 LAB candidate'
}
foreach($dll in @(Get-ChildItem -LiteralPath (Join-Path $prefix 'bin') -Filter '*.dll' -File)) {
    $copy=Join-Path $source $dll.Name
    Require-File $copy
    if((Hash $copy) -ne (Hash $dll.FullName)) {
        throw "WinUI 3 distribution DLL hash mismatch: $($dll.Name)"
    }
}
$cmake=Get-Content -LiteralPath (Join-Path $root 'CMakeLists.txt') -Raw
$version=[regex]::Match($cmake,'(?m)^project\(AXEVoiceDesktop VERSION ([0-9]+\.[0-9]+\.[0-9]+) ').Groups[1].Value
$iss=Get-Content -LiteralPath $spec -Raw
$issVersion=[regex]::Match($iss,'(?m)^AppVersion=([^\r\n]+)').Groups[1].Value
if(!$version -or $version -ne $issVersion){throw 'WinUI 3 installer and CMake product versions differ'}
if($iss -notmatch '(?m)^AppId=AXEVoiceWinUI3NativeLabCandidate\s*$' -or
   $iss -notmatch '(?m)^PrivilegesRequired=lowest\s*$' -or
   $iss -notmatch '(?m)^Name: "desktopicon";.*Flags: unchecked\s*$') {
    throw 'WinUI 3 native lab installer isolation, per-user access, or optional shortcut invalid'
}
if($iss -match '(?m)^\[Run\]') {
    throw 'WinUI 3 laboratory installer must not auto-launch/register SIP'
}
$compiler=Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6/ISCC.exe'
Require-File $compiler
New-Item -ItemType Directory -Force $output | Out-Null
& $compiler "/DWinUI3Source=$source" "/DWinUI3Output=$output" $spec
if($LASTEXITCODE -ne 0) {throw 'WinUI 3 native LAB Inno setup compilation failed'}
$installer=Join-Path $output 'AXE-Voice-WinUI3-Native-Lab-Setup.exe'
Require-File $installer
$desktop=[Environment]::GetFolderPath('DesktopDirectory')
if(!$desktop){throw 'Could not resolve interactive current-user Desktop folder'}
$link=Join-Path $desktop 'AXE Voice WinUI 3 Native Lab.lnk'
if(Test-Path -LiteralPath $link){throw 'WinUI 3 lab test requires no prior desktop shortcut'}
function Invoke-Setup([string[]]$args) {
    $process=Start-Process -FilePath $installer -ArgumentList $args -Wait -PassThru
    try {
        if($process.ExitCode -ne 0) {throw "WinUI 3 lab setup failed with exit $($process.ExitCode)"}
    } finally {$process.Dispose()}
}
function Verify-Payload {
    foreach($file in @(Get-ChildItem -LiteralPath $source -Recurse -File)) {
        $relative=$file.FullName.Substring($source.Length+1)
        $installed=Join-Path $target $relative
        Require-File $installed
        if((Hash $file.FullName) -ne (Hash $installed)) {
            throw "Installed WinUI 3 lab file hash mismatch: $relative"
        }
    }
}
function Invoke-Uninstall {
    $uninstaller=Join-Path $target 'unins000.exe'
    Require-File $uninstaller
    $process=Start-Process -FilePath $uninstaller -ArgumentList @(
        '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART'
    ) -Wait -PassThru
    try {
        if($process.ExitCode -ne 0) {throw "WinUI 3 lab uninstall failed with exit $($process.ExitCode)"}
    } finally {$process.Dispose()}
    foreach($file in @(Get-ChildItem -LiteralPath $source -Recurse -File)) {
        $relative=$file.FullName.Substring($source.Length+1)
        if(Test-Path -LiteralPath (Join-Path $target $relative)) {
            throw "WinUI 3 lab payload survived uninstall: $relative"
        }
    }
    if(Test-Path -LiteralPath $link){throw 'WinUI 3 desktop shortcut survived uninstall'}
}
$common=@('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/DIR=`"$target`"")
try {
    Invoke-Setup $common
    Verify-Payload
    if(Test-Path -LiteralPath $link){throw 'Default installation unexpectedly created a desktop shortcut'}
    Invoke-Uninstall
    # Explicit user opt-in should create the correct executable link and
    # remove it on uninstall, without launching or connecting the softphone.
    Invoke-Setup ($common + @('/TASKS=desktopicon'))
    Verify-Payload
    Require-File $link
    $shell=New-Object -ComObject WScript.Shell
    $shortcut=$shell.CreateShortcut($link)
    try {
        if(![string]::Equals([IO.Path]::GetFullPath($shortcut.TargetPath),
             [IO.Path]::GetFullPath((Join-Path $target 'AXEVoice.WinUI3.exe')),
             [StringComparison]::OrdinalIgnoreCase)) {
            throw 'WinUI 3 desktop shortcut target differs from the installed app'
        }
    } finally {
        if($shortcut){[void][Runtime.InteropServices.Marshal]::ReleaseComObject($shortcut)}
        if($shell){[void][Runtime.InteropServices.Marshal]::ReleaseComObject($shell)}
    }
    Invoke-Uninstall
} finally {
    # Never kill unknown processes, delete user files, or remove a shortcut
    # whose identity was not proved. Failure remains visible to CI.
}
Write-Host "WinUI3 native LAB installer PASS: default off, opt-in shortcut, content hashes, uninstall, version $version."
Get-FileHash -LiteralPath $installer -Algorithm SHA256 | Format-List
