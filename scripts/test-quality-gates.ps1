[CmdletBinding()]
param([ValidateSet('Debug','Release')][string]$Configuration='Release')
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$buildDir=Join-Path $root 'build'
if (-not (Test-Path -LiteralPath $buildDir -PathType Container)) {
    throw 'Build nao existe: execute scripts/build-windows.ps1 antes.'
}
$groups=@('unit','blackbox','integration')
$raw=& ctest --test-dir $buildDir -C $Configuration --show-only=json-v1
if ($LASTEXITCODE -ne 0) { throw 'Nao foi possivel ler o manifesto CTest.' }
$manifest=($raw -join [Environment]::NewLine) | ConvertFrom-Json
$summary=[ordered]@{}
foreach($group in $groups) {
    Write-Host "=== AXE VOICE QUALITY GATE: $group ==="
    $matching=@($manifest.tests | Where-Object {
        $test=$_
        @($test.properties | Where-Object {
            $_.name -eq 'LABELS' -and @($_.value) -contains $group
        }).Count -gt 0
    })
    if ($matching.Count -eq 0) { throw "Gate '$group' sem testes." }
    & ctest --test-dir $buildDir -C $Configuration --output-on-failure -L "^$group$"
    if ($LASTEXITCODE -ne 0) { throw "Gate '$group' FALHOU." }
    $summary[$group]=$matching.Count
    Write-Host "GATE $group APROVADO: $($matching.Count)/$($matching.Count)"
}
$total=0
foreach($n in $summary.Values) { $total+=$n }
Write-Host "AXE Voice - 100% de PASSAGEM dos testes categorizados: $total/$total"
Write-Host 'Isto NAO significa 100% de cobertura de codigo ou homologacao SIP real.'
