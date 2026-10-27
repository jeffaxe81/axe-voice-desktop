[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Source = Get-Content -LiteralPath (Join-Path $Root 'scripts/package-native-candidate.ps1') -Raw
$Boundary = $Source.IndexOf('# A repeated packaging run')
if ($Boundary -lt 0) { throw 'Package provenance gate boundary missing' }
$Fixture = Join-Path ([IO.Path]::GetTempPath()) ('axe-package-path-security-' + [guid]::NewGuid())
function Put([string]$Relative, [string]$Content) {
    $Path = Join-Path $Fixture $Relative
    New-Item -ItemType Directory -Force (Split-Path $Path) | Out-Null
    [IO.File]::WriteAllText($Path, $Content)
}
function Hash([string]$Relative) {
    (Get-FileHash -LiteralPath (Join-Path $Fixture $Relative) -Algorithm SHA256).Hash.ToLowerInvariant()
}
try {
    Put 'scripts/gate.ps1' $Source.Substring(0, $Boundary)
    Put 'installer/axe-native-candidate.iss' "AppVersion=1.2.3`n"
    Put 'build/native-sdk-tls-media/product/CMakeCache.txt' "AXV_ENABLE_NATIVE_SIP:BOOL=ON`nAXV_NATIVE_TLS_MEDIA:BOOL=ON`nCMAKE_PROJECT_VERSION:STATIC=1.2.3`n"
    Put 'build/native-sdk-tls-media/product/Release/AXE-Voice.exe' 'synthetic nonexecutable GUI'
    Put 'build/native-sdk-tls-media/prefix/include/a.h' 'verified synthetic header'
    $Manifest = @{
        gui_native_sdk = 'ON/native candidate'; tls = $true; production_approved = $false
        product_version = '1.2.3'
        product_sha256 = Hash 'build/native-sdk-tls-media/product/Release/AXE-Voice.exe'
        files = @{ 'include/a.h' = Hash 'build/native-sdk-tls-media/prefix/include/a.h' }
    }
    Put 'build/native-sdk-tls-media/prefix/CANDIDATE-MANIFEST.json' ($Manifest | ConvertTo-Json -Depth 4)
    & (Join-Path $Fixture 'scripts/gate.ps1')

    # Recursive enumeration skips directory links, whereas staging enumerates
    # bin directly and follows them. On Windows a junction needs no symlink privilege.
    Put 'outside-prefix/unverified.dll' 'unmanifested synthetic DLL; never executed'
    $Bin = Join-Path $Fixture 'build/native-sdk-tls-media/prefix/bin'
    $LinkType = if ($IsWindows -or $env:OS -eq 'Windows_NT') { 'Junction' } else { 'SymbolicLink' }
    New-Item -ItemType $LinkType -Path $Bin -Target (Join-Path $Fixture 'outside-prefix') | Out-Null
    $StageFiles = @(Get-ChildItem $Bin -Filter '*.dll')
    if ($StageFiles.Count -ne 1 -or $StageFiles[0].Name -ne 'unverified.dll') {
        throw 'Fixture failed to model packaging bin enumeration'
    }
    $Rejected = $false
    try { & (Join-Path $Fixture 'scripts/gate.ps1') }
    catch { $Rejected = $_.Exception.Message -eq 'SDK redirected paths are not allowed' }
    if (!$Rejected) {
        throw 'Package accepted an unmanifested DLL staged through a linked bin directory'
    }
} finally {
    if (Test-Path -LiteralPath $Fixture) { Remove-Item -LiteralPath $Fixture -Recurse -Force }
}
Write-Output 'Package rejects unmanifested DLLs reached through linked bin directories'
