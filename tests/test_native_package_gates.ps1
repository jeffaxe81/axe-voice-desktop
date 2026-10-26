[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Prefix = Join-Path $Root 'build/native-sdk-tls-media/prefix'
$ManifestPath = Join-Path $Prefix 'CANDIDATE-MANIFEST.json'
$Original = [IO.File]::ReadAllBytes($ManifestPath)
$Fixture = Join-Path $Prefix 'bin/axe-unmanifested-gate-fixture.dll'
if (Test-Path $Fixture) { throw 'Package gate fixture already exists' }
function Assert-PackageRejected([string]$Expected) {
    $Failure = $null
    try { & (Join-Path $Root 'scripts/package-native-candidate.ps1') }
    catch { $Failure = $_.Exception.Message }
    if ($Failure -ne $Expected) { throw "Expected '$Expected'; got '$Failure'" }
}
try {
    $Manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json
    $Manifest.product_sha256 = '0' * 64
    $Manifest | ConvertTo-Json -Depth 4 | Set-Content $ManifestPath -Encoding utf8
    Assert-PackageRejected 'Candidate GUI differs from the verified build'
    [IO.File]::WriteAllBytes($ManifestPath, $Original)
    $Manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json
    $Manifest.product_version = 'invalid-package-gate-version'
    $Manifest | ConvertTo-Json -Depth 4 | Set-Content $ManifestPath -Encoding utf8
    Assert-PackageRejected 'Candidate product, manifest and installer versions differ'
    [IO.File]::WriteAllBytes($ManifestPath, $Original)
    [IO.File]::WriteAllText($Fixture, 'unmanifested fixture')
    Assert-PackageRejected 'SDK file differs from manifest: bin/axe-unmanifested-gate-fixture.dll'
} finally {
    [IO.File]::WriteAllBytes($ManifestPath, $Original)
    if (Test-Path $Fixture) { Remove-Item $Fixture -Force }
}
Write-Output 'Candidate package rejects replaced GUI, version mismatch and undeclared DLL'
