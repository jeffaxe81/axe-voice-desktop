[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Source = Get-Content -LiteralPath (Join-Path $Root 'scripts/package-native-candidate.ps1') -Raw
$Boundary = $Source.IndexOf('# A repeated packaging run')
if ($Boundary -lt 0) { throw 'Package provenance gate boundary missing' }
$Fixture = Join-Path ([IO.Path]::GetTempPath()) ('axe-manifest-paths-' + [guid]::NewGuid())
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
    Put 'build/native-sdk-tls-media/prefix/include/a.h' 'synthetic header'
    $Manifest = @{
        gui_native_sdk = 'ON/native candidate'; tls = $true; production_approved = $false
        product_version = '1.2.3'
        product_sha256 = Hash 'build/native-sdk-tls-media/product/Release/AXE-Voice.exe'
        files = @{ 'include/a.h' = Hash 'build/native-sdk-tls-media/prefix/include/a.h' }
    }
    $ManifestPath = 'build/native-sdk-tls-media/prefix/CANDIDATE-MANIFEST.json'
    Put $ManifestPath ($Manifest | ConvertTo-Json -Depth 4)
    & (Join-Path $Fixture 'scripts/gate.ps1')
    # License staging is recursive; every staged file must have build provenance.
    # A license update after the build must not silently enter the installer.
    Put 'build/native-sdk-tls-media/prefix/licenses/libre-LICENSE.txt' 'verified synthetic license'
    $Manifest.files['licenses/libre-LICENSE.txt'] = Hash 'build/native-sdk-tls-media/prefix/licenses/libre-LICENSE.txt'
    Put $ManifestPath ($Manifest | ConvertTo-Json -Depth 4)
    & (Join-Path $Fixture 'scripts/gate.ps1')
    Put 'build/native-sdk-tls-media/prefix/licenses/unmanifested.exe' 'inert synthetic nonexecutable extra file; never executed'
    $Failure = $null
    try { & (Join-Path $Fixture 'scripts/gate.ps1') }
    catch { $Failure = $_.Exception.Message }
    if ($Failure -ne 'SDK file differs from manifest: licenses/unmanifested.exe') { throw 'Package accepted an unmanifested file in recursively staged licenses' }
    Remove-Item -LiteralPath (Join-Path $Fixture 'build/native-sdk-tls-media/prefix/licenses/unmanifested.exe')
    Put 'build/native-sdk-tls-media/prefix/licenses/libre-LICENSE.txt' 'changed synthetic license'
    $Failure = $null
    try { & (Join-Path $Fixture 'scripts/gate.ps1') }
    catch { $Failure = $_.Exception.Message }
    if ($Failure -ne 'SDK manifest file missing or changed: licenses/libre-LICENSE.txt') { throw 'Package accepted a modified manifested license' }
} finally {
    if (Test-Path -LiteralPath $Fixture) { Remove-Item -LiteralPath $Fixture -Recurse -Force }
}
Write-Output 'Package verifies every recursively staged license file'
