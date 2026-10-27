[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Source = Get-Content -LiteralPath (Join-Path $Root 'scripts/package-native-candidate.ps1') -Raw
$Boundary = $Source.IndexOf('# A repeated packaging run')
if ($Boundary -lt 0) { throw 'Package provenance gate boundary missing' }
$Fixture = Join-Path ([IO.Path]::GetTempPath()) ('axe-manifest-types-' + [guid]::NewGuid())
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
    $ValidJson = $Manifest | ConvertTo-Json -Depth 4
    $Cases = @(
        @{Field='tls';Value=@($false,$true)}, @{Field='tls';Value='true'},
        @{Field='tls';Value=1}, @{Field='production_approved';Value='false'},
        @{Field='production_approved';Value=$null},
        @{Field='gui_native_sdk';Value=@('ON/native candidate')},
        @{Field='product_version';Value=123},
        @{Field='product_sha256';Value=@($Manifest.product_sha256)},
        @{Field='files';Value=@('include/a.h')}
    )
    foreach ($Case in $Cases) {
        $Invalid = $ValidJson | ConvertFrom-Json
        $Invalid.($Case.Field) = $Case.Value
        Put $ManifestPath ($Invalid | ConvertTo-Json -Depth 4)
        $Failure = $null
        try { & (Join-Path $Fixture 'scripts/gate.ps1') }
        catch { $Failure = $_.Exception.Message }
        if ($Failure -ne 'Candidate manifest has invalid field types') {
            throw "Package accepted malformed field type: $($Case.Field) (failure: $Failure)"
        }
    }
    $Invalid = $ValidJson | ConvertFrom-Json
    $Invalid.files.'include/a.h' = @($Invalid.files.'include/a.h')
    Put $ManifestPath ($Invalid | ConvertTo-Json -Depth 4)
    $Failure = $null
    try { & (Join-Path $Fixture 'scripts/gate.ps1') }
    catch { $Failure = $_.Exception.Message }
    if ($Failure -ne 'SDK manifest hash must be a string') { throw 'Package accepted nonstring SDK hash' }
    foreach ($RootJson in @('[]','null',('['+$ValidJson+']'))) {
        Put $ManifestPath $RootJson
        $Failure = $null
        try { & (Join-Path $Fixture 'scripts/gate.ps1') }
        catch { $Failure = $_.Exception.Message }
        if ($Failure -ne 'Candidate manifest has invalid field types') { throw 'Package accepted nonobject root container' }
    }
    Put $ManifestPath $ValidJson
    & (Join-Path $Fixture 'scripts/gate.ps1')

} finally {
    if (Test-Path -LiteralPath $Fixture) { Remove-Item -LiteralPath $Fixture -Recurse -Force }
}
Write-Output 'Package enforces scalar provenance types and string SDK hashes'
