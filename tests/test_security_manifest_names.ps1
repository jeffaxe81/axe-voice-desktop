[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Source = Get-Content -LiteralPath (Join-Path $Root 'scripts/package-native-candidate.ps1') -Raw
$Boundary = $Source.IndexOf('# A repeated packaging run')
if ($Boundary -lt 0) { throw 'Package provenance gate boundary missing' }
$Fixture = Join-Path ([IO.Path]::GetTempPath()) ('axe-manifest-names-' + [guid]::NewGuid())
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
    Put 'build/native-sdk-tls-media/prefix/bin/a.dll' 'synthetic nonexecutable library'
    Put 'build/native-sdk-tls-media/prefix/include/nested/a.h' 'synthetic nested header'
    Put 'build/native-sdk-tls-media/prefix/bin/[a].dll' 'synthetic bracket basename'
    $ManifestPath = 'build/native-sdk-tls-media/prefix/CANDIDATE-MANIFEST.json'
    $Manifest = @{
        gui_native_sdk = 'ON/native candidate'; tls = $true; production_approved = $false
        product_version = '1.2.3'
        product_sha256 = Hash 'build/native-sdk-tls-media/product/Release/AXE-Voice.exe'
        files = @{
            'bin/a.dll' = Hash 'build/native-sdk-tls-media/prefix/bin/a.dll'
            'include/nested/a.h' = Hash 'build/native-sdk-tls-media/prefix/include/nested/a.h'
            'bin/[a].dll' = Hash 'build/native-sdk-tls-media/prefix/bin/[a].dll'
        }
    }
    Put $ManifestPath ($Manifest | ConvertTo-Json -Depth 4)
    & (Join-Path $Fixture 'scripts/gate.ps1')
    Put 'build/native-sdk-tls-media/prefix/bin/a.dll' 'modified synthetic library'
    $Failure = $null
    try { & (Join-Path $Fixture 'scripts/gate.ps1') } catch { $Failure = $_.Exception.Message }
    if ($Failure -ne 'SDK manifest file missing or changed: bin/a.dll') { throw 'Package accepted a modified actual SDK library' }
    Put 'build/native-sdk-tls-media/prefix/bin/a.dll' 'synthetic nonexecutable library'
    foreach ($Name in @('../outside.txt', '..\outside.txt', '/absolute', 'C:/drive', 'bin/a.dll:stream',
                         'bin/CON', 'bin/nul.txt', 'bin/COM1.h', 'bin/CON .txt', 'bin/NUL .dll', 'a//b', './a', 'bin/component.', 'bin/component ', 'bin/component./a', 'bin/component /a')) {
        # Invalid hash guarantees the old gate short-circuits before Get-FileHash;
        # the new canonical-name check must reject before even hash validation.
        # Thus this RED test never attempts an outside-file read.
        $Manifest.files[$Name] = 'intentionally-invalid-hash'
        Put $ManifestPath ($Manifest | ConvertTo-Json -Depth 4)
        $Failure = $null
        try { & (Join-Path $Fixture 'scripts/gate.ps1') } catch { $Failure = $_.Exception.Message }
        if ($Failure -ne 'SDK manifest path is not canonical') {
            throw "Manifest name was not rejected before hashing: $Name (observed: $Failure)"
        }
        $Manifest.files.Remove($Name)
    }
} finally {
    if (Test-Path -LiteralPath $Fixture) { Remove-Item -LiteralPath $Fixture -Recurse -Force }
}
Write-Output 'Package rejects noncanonical manifest names before hashing and accepts literal nested/bracket names'
