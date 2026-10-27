[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Source = Get-Content -LiteralPath (Join-Path $Root 'scripts/package-native-candidate.ps1') -Raw
# Run the real provenance gate, stopping before staging, tool discovery or execution.
$Boundary = $Source.IndexOf('# A repeated packaging run')
if ($Boundary -lt 0) { throw 'Package provenance gate boundary missing' }
$Fixture = Join-Path ([IO.Path]::GetTempPath()) ('axe-package-security-' + [guid]::NewGuid())
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
    Put 'build/native-sdk-tls-media/prefix/bin/a.dll' 'verified synthetic DLL'
    $Manifest = @{
        gui_native_sdk = 'ON/native candidate'; tls = $true; production_approved = $false
        product_version = '1.2.3'
        product_sha256 = Hash 'build/native-sdk-tls-media/product/Release/AXE-Voice.exe'
        files = @{ 'bin/a.dll' = Hash 'build/native-sdk-tls-media/prefix/bin/a.dll' }
    }
    Put 'build/native-sdk-tls-media/prefix/CANDIDATE-MANIFEST.json' ($Manifest | ConvertTo-Json -Depth 4)
    & (Join-Path $Fixture 'scripts/gate.ps1')
    # Square brackets are legal Windows filename characters and PowerShell globs.
    # A manifest hash must describe this exact file, even when another file matches it.
    Put 'build/native-sdk-tls-media/prefix/bin/[a].dll' 'modified synthetic DLL'
    $Manifest.files['bin/[a].dll'] = $Manifest.files['bin/a.dll']
    Put 'build/native-sdk-tls-media/prefix/CANDIDATE-MANIFEST.json' ($Manifest | ConvertTo-Json -Depth 4)
    $Failure = $null
    try { & (Join-Path $Fixture 'scripts/gate.ps1') }
    catch { $Failure = $_.Exception.Message }
    if ($Failure -ne 'SDK manifest file missing or changed: bin/[a].dll') {
        throw "Package accepted incorrect hash for literal bracket filename (failure: $Failure)"
    }
    $Manifest.files['bin/[a].dll'] = Hash 'build/native-sdk-tls-media/prefix/bin/[a].dll'
    Put 'build/native-sdk-tls-media/prefix/CANDIDATE-MANIFEST.json' ($Manifest | ConvertTo-Json -Depth 4)
    & (Join-Path $Fixture 'scripts/gate.ps1')

} finally {
    if (Test-Path -LiteralPath $Fixture) { Remove-Item -LiteralPath $Fixture -Recurse -Force }
}
Write-Output 'Package hashes literal filenames and rejects wildcard-alias hash bypass'
